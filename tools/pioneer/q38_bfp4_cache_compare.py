#!/usr/bin/env python3
"""Read-only cross-platform comparison of QGW8 and legacy BFP4 cache payloads.

The result is evidence about bytes, never authorization to re-key, install,
execute, or claim model quality. All SHA checks are recomputed from data.
"""
import hashlib
import json
import os
from pathlib import Path
import re
import stat
import struct
import subprocess
import sys

SOURCE = 'c600de0300ae8a0eb3a6c0b8b5561b8b96f16bd2c863c2a66c42de29d391a747'
HOST = '57f8d22c609d2ba9b9a65be5be19452c35f937ec33f69e65aecd4033b703a39c'
TARGET = '5dc4ffb1257bf99a8b38f7139e5fe95d04e42f11d63beda382d78aeacb1dc45c'
RECIPE = ('pioneer-q38-ffn-bfp4b-v1/32x32-face-major/64-exponents+512-mantissas/'
          'Q4K-decode-then-BFP4B-RNE/no-cpu-products/source-code-sha256=')
NAME = re.compile(r'p100a-weight-v1-[0-9a-f]{64}\.cache\Z')
MARKER = b'Pioneer derived model weights cache v1\n'
CHUNK = 65536


def exact(f, length):
    data = f.read(length)
    if len(data) != length:
        raise ValueError('short cache read')
    return data


def identity(st):
    return (st.st_dev, st.st_ino, st.st_size, st.st_mode, st.st_uid,
            st.st_nlink, st.st_mtime_ns, st.st_ctime_ns)


def inventory(directory, implementation, expected_count):
    root = Path(directory)
    if not root.is_absolute() or root.is_symlink() or not root.is_dir():
        raise ValueError('cache must be an absolute, non-symlink directory')
    if (root / '.p100a-weights-v1').read_bytes() != MARKER:
        raise ValueError('cache marker mismatch')
    recipe = hashlib.sha256((RECIPE + implementation).encode()).digest()
    paths = sorted(p for p in root.iterdir() if NAME.fullmatch(p.name))
    if len(paths) != expected_count:
        raise ValueError('unexpected cache entry count: %d' % len(paths))
    entries = {}
    for number, path in enumerate(paths, 1):
        fd = os.open(str(path), os.O_RDONLY | os.O_NOFOLLOW)
        with os.fdopen(fd, 'rb') as f:
            before = os.fstat(f.fileno())
            if not stat.S_ISREG(before.st_mode) or before.st_nlink != 1:
                raise ValueError('unsafe cache file')
            header = exact(f, 256)
            magic, version, header_bytes, chunk_bytes, reserved = struct.unpack_from('<8sIIII', header)
            size, chunks, entry = struct.unpack_from('<QQQ', header, 24)
            if (magic, version, header_bytes, chunk_bytes, reserved) != (b'P100AWC1', 1, 256, CHUNK, 0):
                raise ValueError('cache header identity mismatch')
            if not 0 < size <= (1 << 30) or chunks != (size + CHUNK - 1) // CHUNK:
                raise ValueError('invalid payload geometry')
            if before.st_size != 256 + chunks * 32 + size:
                raise ValueError('file size mismatch')
            if header[48:80].hex() != SOURCE or header[80:112] != recipe or any(header[176:]):
                raise ValueError('source, recipe or reserved header mismatch')
            key_header = bytearray(header)
            key_header[112:176] = bytes(64)
            if path.name != 'p100a-weight-v1-' + hashlib.sha256(key_header).hexdigest() + '.cache':
                raise ValueError('cache filename/key mismatch')
            table = exact(f, chunks * 32)
            if hashlib.sha256(table).digest() != header[144:176]:
                raise ValueError('chunk table hash mismatch')
            whole = hashlib.sha256()
            for index in range(chunks):
                data = exact(f, min(CHUNK, size - index * CHUNK))
                if hashlib.sha256(data).digest() != table[index * 32:(index + 1) * 32]:
                    raise ValueError('payload chunk hash mismatch')
                whole.update(data)
            if whole.digest() != header[112:144] or f.read(1):
                raise ValueError('payload hash or EOF mismatch')
            if identity(before) != identity(os.fstat(f.fileno())):
                raise ValueError('cache entry changed during read')
            key = '%016x:%d' % (entry, size)
            if key in entries:
                raise ValueError('duplicate logical entry')
            entries[key] = whole.hexdigest()
        if number % 100 == 0 or number == expected_count:
            print('verified %d/%d' % (number, expected_count), file=sys.stderr, flush=True)
    return entries


def compare(host, target):
    missing = set(target) - set(host)
    changed = [key for key, value in target.items() if host.get(key) != value]
    extra = set(host) - set(target)
    layers = {layer: sum(int(key.split(':')[0], 16) >> 48 == layer for key in extra)
              for layer in (48, 49)}
    if missing or changed or len(extra) != 42 or layers != {48: 21, 49: 21}:
        raise ValueError('cross-platform mismatch: missing=%d changed=%d extra=%d layers=%s'
                         % (len(missing), len(changed), len(extra), layers))
    return 'PASS: 1071 matched payloads; 42 host-only entries in layers 48 and 49'


def main():
    if len(sys.argv) == 5 and sys.argv[1] == '--inventory':
        print(json.dumps(inventory(sys.argv[2], sys.argv[3], int(sys.argv[4])), sort_keys=True))
        return
    if len(sys.argv) != 4 or sys.argv[1] != '--compare':
        raise SystemExit('usage: --compare ABS_LOCAL_CACHE ABS_REMOTE_READ_ONLY_CACHE')
    local = inventory(sys.argv[2], HOST, 1113)
    remote = sys.argv[3]
    if remote != '/mnt/haiku-check/home/pioneer-hybrid-bfp4-20261107-cache':
        raise ValueError('remote cache path must match the pinned read-only mount')
    ssh = ['ssh', '-o', 'BatchMode=yes', '-o', 'ConnectTimeout=5',
           '-o', 'StrictHostKeyChecking=yes', '-o',
           'UserKnownHostsFile=/private/tmp/pioneer-fedora-photo-verified-20261107.known_hosts',
           '-i', '/Users/arma/.ssh/haiku_pioneer_ed25519', 'milkv@10.0.0.247']
    command = 'sudo -n python3 - --inventory ' + remote + ' ' + TARGET + ' 1071'
    script = Path(__file__).read_bytes()
    result = subprocess.run(ssh + [command], input=script, stdout=subprocess.PIPE, check=True)
    target = json.loads(result.stdout)
    if len(target) != 1071:
        raise ValueError('invalid remote inventory')
    print(compare(local, target))
    print('READ ONLY: byte equivalence for shared entries only; no re-keying, device execution or speed result')


if __name__ == '__main__':
    main()
