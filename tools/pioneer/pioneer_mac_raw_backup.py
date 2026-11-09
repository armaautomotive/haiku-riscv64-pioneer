#!/usr/bin/env python3
"""One-way, exclusive raw backup of the pinned Pioneer disk to this Mac.

The remote reader verifies disk identity and refuses mounted partitions. The
local writer keeps incomplete files on error and verifies a full readback.
No restore, delete, repair, partition write, or overwrite operation exists.
"""
import hashlib
import os
from pathlib import Path
import shlex
import subprocess
import sys
import tempfile

SIZE = 120034123776
RESERVE = 15 * 1024**3
CHUNK = 8 * 1024**2
DEST = Path('/Users/arma/Documents/ChatGPT/HaikuOS/generated.pioneer')
SSH = ['ssh', '-T', '-o', 'BatchMode=yes', '-o', 'IdentitiesOnly=yes',
       '-o', 'ConnectTimeout=8', '-o', 'StrictHostKeyChecking=yes', '-o',
       'UserKnownHostsFile=/private/tmp/pioneer-fedora-photo-verified-20261107.known_hosts',
       '-i', '/Users/arma/.ssh/haiku_pioneer_ed25519', 'milkv@10.0.0.247']

REMOTE = r'''import hashlib, os, re, stat, subprocess, sys
link='/dev/disk/by-id/ata-Samsung_SSD_840_Series_S14CNSAD300075H'
disk=os.path.realpath(link); part=os.path.realpath(link+'-part1')
def cmd(*a): return subprocess.check_output(a,text=True).strip()
if os.geteuid()!=0 or not re.fullmatch(r'/dev/sd[a-z]',disk) or part!=disk+'1': raise SystemExit('bad disk identity')
ds=os.stat(disk); ps=os.stat(part)
if not stat.S_ISBLK(ds.st_mode) or not stat.S_ISBLK(ps.st_mode): raise SystemExit('not a block device')
if cmd('lsblk','-dn','-o','SERIAL',disk)!='S14CNSAD300075H' or cmd('blkid','-s','UUID','-o','value',part)!='020147de08752551': raise SystemExit('identity mismatch')
if cmd('blockdev','--getsize64',disk)!='120034123776' or cmd('blockdev','--getsize64',part)!='120033640448': raise SystemExit('size mismatch')
if open('/sys/class/block/'+os.path.basename(part)+'/start').read().strip()!='64': raise SystemExit('partition offset mismatch')
if cmd('lsblk','-nr','-o','NAME,TYPE',disk).splitlines()!=[os.path.basename(disk)+' disk',os.path.basename(part)+' part']: raise SystemExit('unexpected partitions')
devices={f'{os.major(v)}:{os.minor(v)}' for v in (ds.st_rdev,ps.st_rdev)}
for ent in os.scandir('/proc'):
 if not ent.name.isdigit(): continue
 try:
  with open('/proc/'+ent.name+'/mountinfo',encoding='ascii') as f:
   if any(line.split()[2] in devices for line in f): raise SystemExit('source mounted')
 except FileNotFoundError: pass
fd=os.open(disk,os.O_RDONLY|os.O_EXCL)
if os.fstat(fd).st_rdev!=ds.st_rdev or os.path.realpath(link)!=disk: raise SystemExit('source changed')
h=hashlib.sha256(); remaining=120034123776
while remaining:
 b=os.read(fd,min(8*1024**2,remaining))
 if not b: raise SystemExit('short read')
 h.update(b); sys.stdout.buffer.write(b); remaining-=len(b)
sys.stdout.buffer.flush(); os.close(fd)
print('SOURCE_SHA256 '+h.hexdigest(),file=sys.stderr,flush=True)
'''


def copy_stream(source, target, size, progress=lambda done: None):
    h = hashlib.sha256()
    done = 0
    while done < size:
        data = source.read(min(CHUNK, size - done))
        if not data:
            raise RuntimeError('short source stream')
        view = memoryview(data)
        while view:
            count = target.write(view)
            if not count:
                raise RuntimeError('short destination write')
            view = view[count:]
        h.update(data)
        done += len(data)
        progress(done)
    if source.read(1):
        raise RuntimeError('extra source bytes')
    return h.hexdigest()


def main():
    if sys.argv[1:] != ['--create']:
        raise SystemExit('usage: pioneer_mac_raw_backup.py --create')
    if DEST.is_symlink() or not DEST.is_dir():
        raise RuntimeError('unsafe destination directory')
    fs = os.statvfs(DEST)
    if fs.f_bavail * fs.f_frsize < SIZE + RESERVE:
        raise RuntimeError('insufficient space including 15 GiB reserve')
    stage = Path(tempfile.mkdtemp(prefix='haiku-postoutage-backup-', dir=DEST))
    os.chmod(stage, 0o700)
    path = stage / 'samsung.raw.partial'
    print('BACKUP_DIRECTORY', stage, flush=True)
    proc = subprocess.Popen(SSH + ['sudo -n python3 -c ' + shlex.quote(REMOTE)],
                            stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    last = [0]
    def progress(done):
        if done == SIZE or done - last[0] >= 4 * 1024**3:
            fs = os.statvfs(stage)
            if fs.f_bavail * fs.f_frsize < RESERVE:
                raise RuntimeError('reserve reached; partial retained')
            print('COPY', done, '/', SIZE, flush=True)
            last[0] = done
    try:
        with path.open('xb', buffering=0) as f:
            os.chmod(path, 0o600)
            digest = copy_stream(proc.stdout, f, SIZE, progress)
            os.fsync(f.fileno())
        stderr = proc.stderr.read().decode('utf-8', 'replace').strip()
        code = proc.wait()
        if code != 0 or stderr != 'SOURCE_SHA256 ' + digest:
            raise RuntimeError('remote reader failed or source digest mismatch: ' + stderr)
        print('COPY_SHA256', digest, flush=True)
        with path.open('rb', buffering=0) as f, open(os.devnull, 'wb') as sink:
            readback = copy_stream(f, sink, SIZE,
                                   lambda n: print('VERIFY', n, '/', SIZE, flush=True)
                                   if n == SIZE or n % (4 * 1024**3) == 0 else None)
        if readback != digest:
            raise RuntimeError('readback mismatch')
        final = stage / 'samsung.raw'
        os.link(path, final)
        print('BACKUP_VERIFIED', final, SIZE, digest, flush=True)
        print('NOTE: partial and final are hard links to the same data; no cleanup performed', flush=True)
    finally:
        if proc.poll() is None:
            proc.terminate()
            proc.wait()


if __name__ == '__main__':
    main()
