#!/usr/bin/env python3
"""Independent PRIVATE Q4 export arithmetic/layout check;NOT hardware authority."""
import contextlib
from fractions import Fraction
import hashlib
import io
import os
import pathlib
import re
import runpy
import stat
import struct
import sys

assert __debug__ and len(sys.argv) == 3
private, original_path = map(pathlib.Path, sys.argv[1:])
assert original_path.is_absolute()
assert private.is_absolute() and not private.is_symlink()
assert private.stat().st_uid == os.geteuid() and stat.S_IMODE(private.stat().st_mode) == 0o700
def identity(info):
    return (info.st_dev, info.st_ino, info.st_size, info.st_uid, info.st_gid,
            info.st_mode, info.st_nlink, info.st_mtime_ns, info.st_ctime_ns)


def read(name):
    fd = os.open(private / name, os.O_RDONLY | os.O_NOFOLLOW | os.O_CLOEXEC)
    try:
        before = os.fstat(fd)
        assert stat.S_ISREG(before.st_mode) and before.st_nlink == 1 and before.st_uid == os.geteuid()
        assert stat.S_IMODE(before.st_mode) == 0o600 and before.st_size < 16 * 1024 * 1024
        value = os.pread(fd, before.st_size + 1, 0)
        assert len(value) == before.st_size and identity(before) == identity(os.fstat(fd))
        return value
    finally:
        os.close(fd)

@contextlib.contextmanager
def authenticated_original(path):
    fd = os.open(path, os.O_RDONLY | os.O_NOFOLLOW | os.O_CLOEXEC)
    try:
        source = os.fstat(fd)
        assert stat.S_ISREG(source.st_mode) and source.st_uid == os.geteuid() and source.st_nlink == 1
        assert not source.st_mode & 0o022 and source.st_size == 18973870528
        digest = hashlib.sha256()
        for offset in range(0, source.st_size, 8 * 1024 * 1024):
            part = os.pread(fd, min(8 * 1024 * 1024, source.st_size - offset), offset)
            assert len(part) == min(8 * 1024 * 1024, source.st_size - offset)
            digest.update(part)
        assert digest.hexdigest() == 'c600de0300ae8a0eb3a6c0b8b5561b8b96f16bd2c863c2a66c42de29d391a747'
        assert identity(source) == identity(os.fstat(fd))
        position = 0

        def take(n):
            nonlocal position
            assert 0 <= n <= 16 * 1024 * 1024 and position + n <= source.st_size
            value = os.pread(fd, n, position)
            assert len(value) == n
            position += n
            return value

        def number(fmt):
            return struct.unpack('<' + fmt, take(struct.calcsize('<' + fmt)))[0]

        def string():
            count = number('Q')
            assert count <= 1024 * 1024
            return take(count).decode('utf8')

        def metadata(kind, depth=0):
            assert depth < 16
            if kind == 8:
                return string()
            if kind == 9:
                element, count = number('I'), number('Q')
                assert count < 2000000
                for _ in range(count):
                    metadata(element, depth + 1)
                return None
            assert kind in (0, 1, 2, 3, 4, 5, 6, 7, 10, 11, 12)
            return number({0: 'B', 1: 'b', 2: 'H', 3: 'h', 4: 'I', 5: 'i', 6: 'f', 7: 'B', 10: 'Q', 11: 'q', 12: 'd'}[kind])

        assert take(4) == b'GGUF' and number('I') == 3
        tensor_count, metadata_count = number('Q'), number('Q')
        assert tensor_count == 851 and 0 < metadata_count < 10000
        alignment, keys = 32, set()
        for _ in range(metadata_count):
            key, kind = string(), number('I')
            assert key not in keys
            keys.add(key)
            value = metadata(kind)
            if key == 'general.alignment':
                assert kind == 4
                alignment = value
        assert 0 < alignment <= 4096 and not alignment & (alignment - 1)
        tensors = []
        for _ in range(tensor_count):
            name, rank = string(), number('I')
            assert 1 <= rank <= 4
            dimensions = [number('Q') for _ in range(rank)]
            kind, offset = number('I'), number('Q')
            tensors.append((name, dimensions, kind, offset))
        start = (position + alignment - 1) & -alignment

        yield fd, source, tensors, start
        assert identity(source) == identity(os.fstat(fd))
    finally:
        os.close(fd)

with authenticated_original(original_path) as (fd, source, tensors, start):
    data = read('PioneerNativeCompactQ4StreamData.h').decode()
    meta = read('PioneerNativeCompactQ4StreamMetadata.h').decode()
    assert 'Version=1,Rows=32,Samples=9,Role=104,PreparedBytes=4608' in data
    def array(text, name, count):
        match = re.search(r'const uint32_t ' + name + r'\[' + str(count) + r'\]=\{([^}]+)\};', text)
        assert match
        words = re.findall(r'0x([0-9a-f]{8})', match[1])
        assert len(words) == count
        return b''.join(struct.pack('<I', int(word, 16)) for word in words)
    with contextlib.redirect_stdout(io.StringIO()):
        rational = runpy.run_path(str(pathlib.Path(__file__).parent / 'pioneer_native_bfp_arithmetic_test.py'))
    def half(bits):
        exponent, mantissa = (bits >> 10) & 31, bits & 1023
        assert exponent != 31
        return (-1 if bits & 0x8000 else 1) * (1024 + mantissa if exponent else mantissa) * rational['power_of_two'](exponent - 25 if exponent else -24)
    def fp32(value, negative_zero=False):
        return rational['encode_normal'](value) if value else (0x80000000 if negative_zero else 0)
    predictions = weights = 0
    for sample in range(9):
        width = 5120 if sample < 6 else 17408
        blocks = width // 256
        ordinal = 7 if sample < 3 else 8 if sample < 6 else 6
        first_row = (0, 8704, 17376)[sample % 3] if sample < 6 else (0, 2560, 5088)[sample % 3]
        name, dims, kind, offset = tensors[ordinal]
        assert (name, dims, kind) == (('blk.0.ffn_gate.weight' if sample < 3 else 'blk.0.ffn_up.weight' if sample < 6 else 'blk.0.ffn_down.weight'), [width, 17408 if sample < 6 else 5120], 12)
        assert first_row + 32 <= dims[1]
        raw, packed, vector, expected = [read(f'sample{sample}.{name}.bin') for name in ('original', 'prepared', 'input', 'expected')]
        assert list(map(len, (raw, packed, vector, expected))) == [blocks*4608, blocks*4608, blocks*512, 128]
        assert array(data, f'p{sample}', blocks*1152) == packed and array(data, f'i{sample}', blocks*128) == vector
        assert array(meta, f'g{sample}', 32) == expected
        totals = [0]*32
        for block in range(blocks):
            original = raw[block*4608:(block+1)*4608]
            prepared = packed[block*4608:(block+1)*4608]
            inputs = vector[block*512:(block+1)*512]
            rebuilt = bytearray(4608)
            for row in range(32):
                q = original[row * 144:(row + 1) * 144]
                source_offset = start + offset + ((first_row + row) * blocks + block) * 144
                assert source_offset + 144 <= source.st_size and os.pread(fd, 144, source_offset) == q
                d_bits, min_bits = struct.unpack_from('<HH', q)
                d, minimum = half(d_bits), half(min_bits)
                for byte in range(128):
                    rebuilt[(row // 16) * 2048 + byte * 16 + row % 16] = q[16 + byte]
                for byte in range(16):
                    rebuilt[4096+(row//16)*256+byte*16+row%16] = q[byte]
                a, b = [], []
                for group in range(8):
                    if group < 4:
                        scale, low = q[4 + group] & 63, q[8 + group] & 63
                    else:
                        scale = (q[8 + group] & 15) | ((q[group] >> 6) << 4)
                        low = (q[8 + group] >> 4) | ((q[4 + group] >> 6) << 4)
                    for column in range(32):
                        c = group * 32 + column
                        quant = (q[16 + (group // 2) * 32 + column] >> (4 * (group & 1))) & 15
                        value = fp32(d * scale * quant - minimum * low)
                        high, tail = divmod(value, 65536)
                        high += int(tail > 32768 or (tail == 32768 and high & 1))
                        a.append(high)
                        bits = 0x3e80 + (c * 19 + block * 11) % 384 + (0x8000 if c & 3 == 2 else 0)
                        assert struct.unpack_from('<H', inputs, c * 2)[0] == bits
                        b.append(bits)
                total = totals[row]
                for first in range(0, 256, 16):
                    for phase in range(4):
                        total = rational['instruction'](a[first:first + 16], b[first:first + 16], phase, total)
                        predictions += 1
                totals[row] = total
            assert prepared == rebuilt
            weights += 8192
            assert identity(source) == identity(os.fstat(fd))
        assert list(struct.unpack('<32I', expected)) == totals, sample
        print(f'PASS PRIVATE compact full-K export sample{sample} K{width} same-open original/all32 Fraction predictions/exact prepared headers/input;sha' + hashlib.sha256(packed+vector).hexdigest(), flush=True)
    assert weights == 2654208 and predictions == 663552
    print('PASS PRIVATE independent Fraction compact full-K export:2654208 original weights/663552 rounded phase predictions/288 goldens for576 two-job words;NO CXX codec/preparer/FPU oracle shared;full original SHA/nanosecond FD identity verified;NOT native/hardware/quality/speed authority')
