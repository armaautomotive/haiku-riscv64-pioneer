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

assert __debug__ and len(sys.argv) == 2
private = pathlib.Path(sys.argv[1])
assert private.is_absolute() and not private.is_symlink()
assert private.stat().st_uid == os.geteuid() and stat.S_IMODE(private.stat().st_mode) == 0o700
def read(name):
    path = private / name
    info = path.lstat()
    assert stat.S_ISREG(info.st_mode) and info.st_nlink == 1 and info.st_uid == os.geteuid()
    assert stat.S_IMODE(info.st_mode) == 0o600
    return path.read_bytes()
data = read('PioneerNativeQ4FpuStreamData.h').decode()
meta = read('PioneerNativeQ4FpuStreamMetadata.h').decode()
assert 'Version=1,Rows=32,Samples=9,Role=86' in data
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
    raw, packed, vector, expected = [read(f'sample{sample}.{name}.bin') for name in ('original', 'prepared', 'input', 'expected')]
    assert list(map(len, (raw, packed, vector, expected))) == [blocks*4608, blocks*6144, blocks*512, 128]
    assert array(data, f'p{sample}', blocks*1536) == packed and array(data, f'i{sample}', blocks*128) == vector
    assert array(meta, f'g{sample}', 32) == expected
    totals = [0]*32
    for block in range(blocks):
        original = raw[block*4608:(block+1)*4608]
        prepared = packed[block*6144:(block+1)*6144]
        inputs = vector[block*512:(block+1)*512]
        rebuilt = bytearray(6144)
        for row in range(32):
            q = original[row * 144:(row + 1) * 144]
            d_bits, min_bits = struct.unpack_from('<HH', q)
            d, minimum = half(d_bits), half(min_bits)
            for byte in range(128):
                rebuilt[(row // 16) * 2048 + byte * 16 + row % 16] = q[16 + byte]
            a, b = [], []
            for group in range(8):
                if group < 4:
                    scale, low = q[4 + group] & 63, q[8 + group] & 63
                else:
                    scale = (q[8 + group] & 15) | ((q[group] >> 6) << 4)
                    low = (q[8 + group] >> 4) | ((q[4 + group] >> 6) << 4)
                coefficient = fp32(d * scale, bool(d_bits & 0x8000))
                negative = fp32(minimum * low, bool(min_bits & 0x8000)) ^ 0x80000000
                for kind, bits in enumerate((coefficient, negative)):
                    at = 4096 + (group * 4 + (row // 16) * 2 + kind) * 64 + (row % 16) * 4
                    struct.pack_into('<I', rebuilt, at, bits)
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
    assert list(struct.unpack('<32I', expected)) == totals, sample
    print(f'PASS PRIVATE full-K export sample{sample} K{width} all32 Fraction predictions/exact prepared headers/input;sha' + hashlib.sha256(packed+vector).hexdigest(), flush=True)
assert weights == 2654208 and predictions == 663552
print('PASS PRIVATE independent Fraction full-K export:2654208 original weights/663552 rounded phase predictions/288 goldens for576 two-job words;NO CXX codec/preparer/FPU oracle shared;NOT original-authentication/native/hardware/quality/speed authority')
