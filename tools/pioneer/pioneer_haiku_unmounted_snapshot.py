#!/usr/bin/env python3
"""Private, exclusive, verified raw backup of the pinned unmounted Haiku partition.

Run only on the Pioneer Linux recovery system. No device is opened writable;
there is deliberately no restore, delete, overwrite, power or mount operation.
An interrupted or failed run leaves its uniquely named partial files in place.
"""
import hashlib
import os
from pathlib import Path
import re
import stat
import subprocess
import sys

SOURCE_DISK_LINK = '/dev/disk/by-id/ata-Samsung_SSD_840_Series_S14CNSAD300075H'
SOURCE_PARTITION_LINK = SOURCE_DISK_LINK + '-part1'
SOURCE_SERIAL = 'S14CNSAD300075H'
SOURCE_UUID = '020147de08752551'
SOURCE_SIZE = 120033640448
DESTINATION = Path('/mnt/ssd')
DESTINATION_DISK_LINK = '/dev/disk/by-id/ata-R3SL480G_E201610050040028'
DESTINATION_PARTITION_LINK = DESTINATION_DISK_LINK + '-part2'
DESTINATION_SERIAL = 'E201610050040028'
STAGE = DESTINATION / 'pioneer-haiku-raw-backup-20261107-a'
RESERVE = 60 * 1024**3
BUFFER = 8 * 1024**2
PROGRESS = 4 * 1024**3


def output(*command):
    return subprocess.check_output(command, text=True).strip()


def mounted_device(device):
    number = '%d:%d' % (os.major(device), os.minor(device))
    # Check all visible process mount namespaces, not only the SSH shell's.
    # A disappearing process is harmless; an unreadable live one is not.
    for entry in os.scandir('/proc'):
        if not entry.name.isdigit():
            continue
        try:
            with open('/proc/' + entry.name + '/mountinfo', encoding='ascii') as mounts:
                if any(line.split()[2] == number for line in mounts):
                    return True
        except FileNotFoundError:
            continue
    return False


def check_devices():
    if sys.platform != 'linux' or os.geteuid() != 0:
        raise RuntimeError('requires root on Linux')
    source_disk = os.path.realpath(SOURCE_DISK_LINK)
    source_partition = os.path.realpath(SOURCE_PARTITION_LINK)
    destination_disk = os.path.realpath(DESTINATION_DISK_LINK)
    destination_partition = os.path.realpath(DESTINATION_PARTITION_LINK)
    if (not re.fullmatch(r'/dev/sd[a-z]', source_disk)
            or not re.fullmatch(r'/dev/sd[a-z]', destination_disk)
            or source_disk == destination_disk
            or source_partition != source_disk + '1'
            or destination_partition != destination_disk + '2'):
        raise RuntimeError('unexpected recorded disk/partition ancestry')
    source = os.stat(source_partition)
    if not stat.S_ISBLK(source.st_mode) or mounted_device(source.st_rdev):
        raise RuntimeError('source is not the expected unmounted block device')
    if output('lsblk', '-dn', '-o', 'SERIAL', source_disk) != SOURCE_SERIAL:
        raise RuntimeError('source serial mismatch')
    if output('blkid', '-s', 'UUID', '-o', 'value', source_partition) != SOURCE_UUID:
        raise RuntimeError('source filesystem identity mismatch')
    if int(output('blockdev', '--getsize64', source_disk)) != 120034123776:
        raise RuntimeError('source disk size mismatch')
    if int(output('blockdev', '--getsize64', source_partition)) != SOURCE_SIZE:
        raise RuntimeError('source size mismatch')
    if Path('/sys/class/block/' + Path(source_partition).name + '/start').read_text().strip() != '64':
        raise RuntimeError('source partition offset mismatch')
    if output('lsblk', '-dn', '-o', 'SERIAL', destination_disk) != DESTINATION_SERIAL:
        raise RuntimeError('destination serial mismatch')
    if output('blkid', '-s', 'UUID', '-o', 'value', destination_partition) != 'd7acf635-9bfa-43b5-abf9-a35bf5abfd82':
        raise RuntimeError('destination filesystem identity mismatch')
    mount = output('findmnt', '-rn', '-T', str(DESTINATION), '-o', 'SOURCE,FSTYPE,OPTIONS').split()
    if len(mount) != 3 or mount[:2] != [destination_partition, 'ext4'] or 'rw' not in mount[2].split(','):
        raise RuntimeError('destination mount mismatch')
    if DESTINATION.is_symlink() or not DESTINATION.is_dir():
        raise RuntimeError('unsafe destination')
    destination = os.stat(destination_partition)
    if not stat.S_ISBLK(destination.st_mode) or os.stat(DESTINATION).st_dev != destination.st_rdev:
        raise RuntimeError('destination mount device mismatch')
    if destination.st_rdev == source.st_rdev:
        raise RuntimeError('source and destination coincide')
    return source_partition, source


def free_bytes(path):
    v = os.statvfs(path)
    return v.f_bavail * v.f_frsize


def write_all(fd, data):
    view = memoryview(data)
    while view:
        n = os.write(fd, view)
        if n <= 0:
            raise OSError('short backup write')
        view = view[n:]


def copy_exact(source, destination, size, on_progress=lambda done: None):
    digest = hashlib.sha256()
    done = 0
    while done < size:
        data = os.read(source, min(BUFFER, size - done))
        if not data:
            raise OSError('short source read')
        write_all(destination, data)
        digest.update(data)
        done += len(data)
        on_progress(done)
    return digest.hexdigest()


def hash_exact(fd, size, on_progress=lambda done: None):
    digest = hashlib.sha256()
    done = 0
    while done < size:
        data = os.read(fd, min(BUFFER, size - done))
        if not data:
            raise OSError('short backup read')
        digest.update(data)
        done += len(data)
        on_progress(done)
    if os.read(fd, 1):
        raise OSError('backup has unexpected trailing data')
    return digest.hexdigest()


def progress(label, reserve_check=False):
    last = 0
    def observe(done):
        nonlocal last
        if done == SOURCE_SIZE or done - last >= PROGRESS:
            if reserve_check and free_bytes(DESTINATION) < RESERVE:
                raise OSError('destination reserve reached; incomplete backup retained')
            print('%s %d/%d' % (label, done, SOURCE_SIZE), flush=True)
            last = done
    return observe


def main():
    if sys.argv[1:] != ['--create-pinned-snapshot']:
        raise SystemExit('usage: --create-pinned-snapshot')
    source_path, source_info = check_devices()
    if free_bytes(DESTINATION) < SOURCE_SIZE + RESERVE:
        raise RuntimeError('not enough free space plus 60 GiB reserve')
    os.umask(0o077)
    os.mkdir(STAGE, 0o700)  # exclusively new; never reused or removed here
    directory = os.open(STAGE, os.O_RDONLY | os.O_DIRECTORY | os.O_NOFOLLOW)
    src = dst = -1
    try:
        src = os.open(source_path, os.O_RDONLY | os.O_NOFOLLOW)
        if os.fstat(src).st_rdev != source_info.st_rdev or mounted_device(source_info.st_rdev):
            raise RuntimeError('source identity changed or became mounted')
        dst = os.open('haiku-sda1.raw.incomplete', os.O_RDWR | os.O_CREAT | os.O_EXCL | os.O_NOFOLLOW,
                      0o600, dir_fd=directory)
        source_hash = copy_exact(src, dst, SOURCE_SIZE, progress('COPY', True))
        os.fsync(dst)
        if os.fstat(dst).st_size != SOURCE_SIZE:
            raise RuntimeError('destination length mismatch')
        os.lseek(dst, 0, os.SEEK_SET)
        backup_hash = hash_exact(dst, SOURCE_SIZE, progress('VERIFY'))
        if backup_hash != source_hash:
            raise RuntimeError('backup SHA-256 mismatch')
        final_path, final_info = check_devices()
        if final_path != source_path or final_info.st_rdev != source_info.st_rdev:
            raise RuntimeError('device identity changed during backup')
        verified = os.fstat(dst)
        named = os.stat('haiku-sda1.raw.incomplete', dir_fd=directory, follow_symlinks=False)
        if (verified.st_dev, verified.st_ino, verified.st_nlink) != (named.st_dev, named.st_ino, 1):
            raise RuntimeError('backup file identity changed before publication')
        # link() refuses an existing destination, unlike rename(). The old
        # name is removed only after the verified inode has a second name.
        os.link('haiku-sda1.raw.incomplete', 'haiku-sda1.raw', src_dir_fd=directory,
                dst_dir_fd=directory, follow_symlinks=False)
        published = os.stat('haiku-sda1.raw', dir_fd=directory, follow_symlinks=False)
        if (published.st_dev, published.st_ino) != (verified.st_dev, verified.st_ino):
            raise RuntimeError('published backup identity mismatch')
        os.unlink('haiku-sda1.raw.incomplete', dir_fd=directory)
        os.fsync(directory)
        manifest = os.open('haiku-sda1.raw.sha256', os.O_WRONLY | os.O_CREAT | os.O_EXCL | os.O_NOFOLLOW,
                           0o600, dir_fd=directory)
        try:
            write_all(manifest, (source_hash + '  haiku-sda1.raw\n').encode('ascii'))
            os.fsync(manifest)
        finally:
            os.close(manifest)
        os.fsync(directory)
        print('SNAPSHOT_VERIFIED %s bytes=%d sha256=%s' % (STAGE, SOURCE_SIZE, source_hash), flush=True)
        print('Read-only source; same-host backup, no restore, power-loss or disk-health guarantee.', flush=True)
    finally:
        if src >= 0:
            os.close(src)
        if dst >= 0:
            os.close(dst)
        os.close(directory)


if __name__ == '__main__':
    main()
