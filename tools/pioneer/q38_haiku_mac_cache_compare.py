#!/usr/bin/env python3
"""Compare real independently prepared cache payloads; read-only, no device IO."""
import hashlib
import os
from pathlib import Path
import re
import shlex
import struct
import subprocess

HERE = Path(__file__).resolve().parents[2]
SOURCE = bytes.fromhex('c600de0300ae8a0eb3a6c0b8b5561b8b96f16bd2c863c2a66c42de29d391a747')
SPECS = (
    ('BFP4', HERE / 'generated.pioneer/qgw8-host-bfp4-20261107',
     '/HaikuData/pioneer-qgw8-bfp4-native-20261107',
     'pioneer-q38-ffn-bfp4b-v1/32x32-face-major/64-exponents+512-mantissas/Q4K-decode-then-BFP4B-RNE/no-cpu-products/source-code-sha256=',
     '57f8d22c609d2ba9b9a65be5be19452c35f937ec33f69e65aecd4033b703a39c',
     '5dc4ffb1257bf99a8b38f7139e5fe95d04e42f11d63beda382d78aeacb1dc45c', 42),
    ('HEAD', HERE / 'generated.pioneer/qgw8-host-head-bfp8-20261107',
     '/HaikuData/pioneer-qgw8-head-native-20261107',
     'pioneer-q38-output-head-bfp8b-v1/32x32-face-major/64-exponents+1024-mantissas/Q6K-decode-then-BFP8B-RNE/no-cpu-products/source-code-sha256=',
     '44c4d4b8727890b9a880f2a0e6954a2e2cb306200700f047eec85944e2066bed',
     'e0e45dce6f1d95124aaeb956e2ee4076aff5b11d35d8079e205054cf33cf316f', 7),
)
NAME = re.compile(r'p100a-weight-v1-[0-9a-f]{64}\.cache\Z')


def collect(spec):
    label, directory, remote, recipe, host_key, target_key, count = spec
    source_recipe = hashlib.sha256((recipe + host_key).encode()).digest()
    target_recipe = hashlib.sha256((recipe + target_key).encode()).digest()
    selected = []
    for path in sorted(directory.iterdir()):
        if not NAME.fullmatch(path.name):
            continue
        fd = os.open(path, os.O_RDONLY | os.O_NOFOLLOW)
        with os.fdopen(fd, 'rb') as file:
            before = os.fstat(file.fileno())
            header = file.read(256)
            if len(header) != 256 or header[:8] != b'P100AWC1':
                raise ValueError('bad cache header')
            version, hsize, chunk, reserved = struct.unpack_from('<IIII', header, 8)
            size, chunks, entry = struct.unpack_from('<QQQ', header, 24)
            if label == 'BFP4' and entry >> 48 not in (48, 49):
                continue
            if (version, hsize, chunk, reserved) != (1, 256, 65536, 0) or not 0 < size <= (1 << 30):
                raise ValueError('bad cache geometry')
            if chunks != (size + 65535) // 65536 or before.st_size != 256 + 32 * chunks + size:
                raise ValueError('cache length mismatch')
            if header[48:80] != SOURCE or header[80:112] != source_recipe or any(header[176:]):
                raise ValueError('cache source/recipe mismatch')
            key = bytearray(header)
            key[112:176] = bytes(64)
            if path.name != 'p100a-weight-v1-' + hashlib.sha256(key).hexdigest() + '.cache':
                raise ValueError('invalid cache key')
            table = file.read(chunks * 32)
            if len(table) != chunks * 32 or hashlib.sha256(table).digest() != header[144:176]:
                raise ValueError('bad chunk table')
            payload = hashlib.sha256()
            for i in range(chunks):
                data = file.read(min(65536, size - i * 65536))
                if hashlib.sha256(data).digest() != table[i * 32:(i + 1) * 32]:
                    raise ValueError('bad payload chunk')
                payload.update(data)
            if payload.digest() != header[112:144] or file.read(1):
                raise ValueError('bad payload digest/length')
            file.seek(112)
            suffix = hashlib.sha256()
            for data in iter(lambda: file.read(1024 * 1024), b''):
                suffix.update(data)
            after = os.fstat(file.fileno())
            if (before.st_dev, before.st_ino, before.st_size, before.st_mtime_ns, before.st_ctime_ns) != \
               (after.st_dev, after.st_ino, after.st_size, after.st_mtime_ns, after.st_ctime_ns):
                raise ValueError('cache changed while reading')
            key[80:112] = target_recipe
            target = 'p100a-weight-v1-' + hashlib.sha256(key).hexdigest() + '.cache'
            selected.append((label, entry, remote + '/' + target, before.st_size, suffix.hexdigest(),
                             hashlib.sha256(key[:112]).hexdigest()))
    if len(selected) != count or len({(x[1], x[3]) for x in selected}) != count:
        raise ValueError('missing or duplicate logical cache entries')
    if label == 'BFP4' and [sum((x[1] >> 48) == layer for x in selected) for layer in (48, 49)] != [21, 21]:
        raise ValueError('unexpected BFP4 layer selection')
    return selected


def compare(entries):
    lines = ['set -eu', 'set -o pipefail']
    for i, (_, _, path, size, _, _) in enumerate(entries):
        quoted = shlex.quote(path)
        lines += [f'test -f {quoted} && test ! -L {quoted}',
                  f'test "$(stat -c %s {quoted})" = {size}',
                  f'before=$(stat -c %d:%i:%s:%Y:%Z {quoted})',
                  f'prefix=$(dd if={quoted} bs=112 count=1 status=none | sha256sum)',
                  f'digest=$(dd if={quoted} bs=1048576 iflag=skip_bytes skip=112 status=none | sha256sum)',
                  f'test "$before" = "$(stat -c %d:%i:%s:%Y:%Z {quoted})"',
                  f'printf "%s %s %s\\n" {i} "${{prefix%% *}}" "${{digest%% *}}"']
    ssh = ['ssh', '-o', 'BatchMode=yes', '-o', 'ConnectTimeout=5', '-o', 'ServerAliveInterval=20',
           '-o', 'StrictHostKeyChecking=yes', '-o',
           'UserKnownHostsFile=/private/tmp/pioneer-haiku-preverified-20261107.known_hosts',
           '-i', '/Users/arma/.ssh/haiku_pioneer_ed25519', 'baron@10.0.0.247', 'bash -s']
    result = subprocess.run(ssh, input='\n'.join(lines) + '\n', text=True, capture_output=True, check=True)
    output = result.stdout.splitlines()
    if len(output) != len(entries):
        raise ValueError('remote comparison returned the wrong count')
    for i, (line, item) in enumerate(zip(output, entries)):
        if line != '%d %s %s' % (i, item[5], item[4]):
            raise ValueError('cross-platform mismatch at %s entry %d' % (item[0], i))
    return len(entries)


def main():
    entries = []
    for spec in SPECS:
        entries.extend(collect(spec))
        print('HOST_VERIFIED', spec[0], spec[-1], flush=True)
    print('PASS', compare(entries), 'full cache suffix hashes match; 42 BFP4 and 7 head; no P100A execution')


if __name__ == '__main__':
    main()
