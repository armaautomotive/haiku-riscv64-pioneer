#!/usr/bin/env python3
"""Test snapshot copying with disposable files only; no disks or SSH."""
import hashlib
import importlib.util
import os
from pathlib import Path
import stat
import tempfile
from unittest.mock import patch
from types import SimpleNamespace

source = Path(__file__).resolve().parents[1] / 'pioneer_haiku_unmounted_snapshot.py'
spec = importlib.util.spec_from_file_location('haiku_snapshot', source)
mod = importlib.util.module_from_spec(spec)
spec.loader.exec_module(mod)
mod.BUFFER = 4096


def fails(action):
    try:
        action()
    except OSError:
        return
    raise AssertionError('unsafe or corrupt case accepted')


with tempfile.TemporaryDirectory(prefix='haiku-snapshot-test-') as temp:
    source_file, backup_file = Path(temp) / 'source', Path(temp) / 'backup'
    contents = bytes(range(256)) * 100
    source_file.write_bytes(contents)
    src = os.open(source_file, os.O_RDONLY)
    dst = os.open(backup_file, os.O_RDWR | os.O_CREAT | os.O_EXCL, 0o600)
    try:
        expected = hashlib.sha256(contents).hexdigest()
        assert mod.copy_exact(src, dst, len(contents)) == expected
        os.lseek(dst, 0, os.SEEK_SET)
        assert mod.hash_exact(dst, len(contents)) == expected
        os.lseek(src, 0, os.SEEK_SET)
        os.lseek(dst, 0, os.SEEK_SET)
        fails(lambda: mod.copy_exact(src, dst, len(contents) + 1))
        os.lseek(dst, 0, os.SEEK_SET)
        fails(lambda: mod.hash_exact(dst, len(contents) - 1))
        os.lseek(dst, 0, os.SEEK_SET)
        fails(lambda: mod.hash_exact(dst, len(contents) + 1))
        with patch.object(mod.os, 'write', return_value=0):
            fails(lambda: mod.write_all(dst, b'anything'))
        os.lseek(src, 0, os.SEEK_SET)
        fails(lambda: mod.copy_exact(src, dst, len(contents), lambda _: (_ for _ in ()).throw(OSError('stop'))))
    finally:
        os.close(src)
        os.close(dst)
assert not (Path(temp) / 'backup').exists()


def check_identity(source='/dev/sdb', destination='/dev/sdc', bad=None):
    paths = {mod.SOURCE_DISK_LINK: source, mod.SOURCE_PARTITION_LINK: source + '1',
             mod.DESTINATION_DISK_LINK: destination, mod.DESTINATION_PARTITION_LINK: destination + '2'}
    if bad == 'ancestry':
        paths[mod.SOURCE_PARTITION_LINK] = '/dev/sda1'
    def simulated_output(*args):
        if args[0] == 'lsblk':
            if args[1] == '-nr':
                rows = Path(source).name + ' disk\n' + Path(source + '1').name + ' part'
                return rows + ('\nextra part' if bad == 'layout' else '')
            if args[-1] == source:
                return 'WRONG' if bad == 'source_serial' else mod.SOURCE_SERIAL
            return 'WRONG' if bad == 'destination_serial' else mod.DESTINATION_SERIAL
        if args[0] == 'blkid':
            if args[-1] == source + '1':
                return 'WRONG' if bad == 'source_uuid' else mod.SOURCE_UUID
            return 'WRONG' if bad == 'destination_uuid' else 'd7acf635-9bfa-43b5-abf9-a35bf5abfd82'
        if args[0] == 'blockdev':
            return str(mod.SOURCE_SIZE if args[-1] == source else 1 if bad == 'size' else mod.PARTITION_SIZE)
        if args[0] == 'findmnt':
            return ('/dev/sda2' if bad == 'mount' else destination + '2') + ' ext4 rw,relatime'
        raise AssertionError(args)
    fake_source = SimpleNamespace(st_mode=stat.S_IFBLK, st_rdev=os.makedev(8, 16))
    fake_partition = SimpleNamespace(st_mode=stat.S_IFBLK, st_rdev=os.makedev(8, 17))
    fake_destination = SimpleNamespace(st_mode=stat.S_IFBLK, st_rdev=os.makedev(8, 34),
                                       st_dev=os.makedev(8, 35 if bad == 'destination_device' else 34))
    def fake_stat(path):
        return fake_source if str(path) == source else fake_partition if str(path) == source + '1' else fake_destination
    with patch.object(mod.sys, 'platform', 'linux'), patch.object(mod.os, 'geteuid', return_value=0), \
         patch.object(mod.os.path, 'realpath', side_effect=lambda path: paths[path]), \
         patch.object(mod.os, 'stat', side_effect=fake_stat), \
         patch.object(mod, 'mounted_device', return_value=bad == 'mounted'), \
         patch.object(mod, 'output', side_effect=simulated_output), \
         patch.object(Path, 'read_text', return_value='65' if bad == 'offset' else '64'), \
         patch.object(Path, 'is_symlink', return_value=False), patch.object(Path, 'is_dir', return_value=True):
        return mod.check_devices()


assert check_identity()[0] == '/dev/sdb'
assert check_identity('/dev/sda', '/dev/sdb')[0] == '/dev/sda'
for invalid in ('ancestry', 'source_serial', 'destination_serial', 'source_uuid', 'destination_uuid',
                'size', 'offset', 'mount', 'mounted', 'destination_device', 'layout'):
    try:
        check_identity(bad=invalid)
    except RuntimeError:
        pass
    else:
        raise AssertionError('bad identity accepted: ' + invalid)
print('PASS: disposable copy/read-back plus reordered disk and identity refusals; no device access')
