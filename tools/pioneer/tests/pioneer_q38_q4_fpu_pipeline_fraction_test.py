#!/usr/bin/env python3
"""Independent Fraction prediction of CLOSED synthetic actual-RV32 results."""
import contextlib
from fractions import Fraction
import io
import pathlib
import re
import runpy
import sys

assert __debug__ and len(sys.argv) == 2
text = pathlib.Path(sys.argv[1]).read_text()
assert text.count('PASS actual RV32I packed-Q4/SFPU/BF16/FPU K256 pipeline:5 two-job trials/all64 words exact,13 failure/numerical controls,') == 1
words = {}
for line in text.splitlines():
    if not line.startswith('Q4_FPU_PIPELINE_REFERENCE'):
        continue
    match = re.fullmatch(r'Q4_FPU_PIPELINE_REFERENCE trial0 pass([01]) row(\d+) word([0-9a-f]{8})', line)
    assert match
    job, row = map(int, match.group(1, 2))
    assert row < 32 and (job, row) not in words
    words[job, row] = int(match.group(3), 16)
assert set(words) == {(job, row) for job in range(2) for row in range(32)}
with contextlib.redirect_stdout(io.StringIO()):
    rational = runpy.run_path(str(pathlib.Path(__file__).parent / 'pioneer_native_bfp_arithmetic_test.py'))

seed = 0x712f3b49
original = []
for i in range(32 * 144):
    seed ^= (seed << 13) & 0xffffffff
    seed ^= seed >> 17
    seed ^= (seed << 5) & 0xffffffff
    original.append(seed & 255)
for row in range(32):
    for at in (0, 2):
        if original[row * 144 + at + 1] & 0x7c == 0x7c:
            original[row * 144 + at + 1] ^= 4


def half(bits):
    exponent, mantissa = (bits >> 10) & 31, bits & 1023
    assert exponent != 31
    return (-1 if bits & 0x8000 else 1) * (1024 + mantissa if exponent else mantissa) * rational['power_of_two'](exponent - 25 if exponent else -24)


commands = 0
for row in range(32):
    q = original[row * 144:(row + 1) * 144]
    d, dmin = half(q[0] | (q[1] << 8)), half(q[2] | (q[3] << 8))
    a, b = [], []
    for c in range(256):
        group, column = divmod(c, 32)
        if group < 4:
            scale, minimum = q[4 + group] & 63, q[8 + group] & 63
        else:
            scale = (q[8 + group] & 15) | ((q[group] >> 6) << 4)
            minimum = (q[8 + group] >> 4) | ((q[4 + group] >> 6) << 4)
        quant = (q[16 + (group // 2) * 32 + column] >> (4 * (group & 1))) & 15
        f32 = rational['encode_normal'](d * scale * quant - dmin * minimum)
        high, tail = divmod(f32, 65536)
        high += int(tail > 32768 or (tail == 32768 and high & 1))
        a.append(high)
        b.append(0x3e80 + (c * 19) % 384 + (0x8000 if c & 3 == 2 else 0))
    total = 0
    for first in range(0, 256, 16):
        for phase in range(4):
            total = rational['instruction'](a[first:first + 16], b[first:first + 16], phase, total)
            commands += 1
    for job in range(2):
        assert total == words[job, row], (job, row, hex(total), hex(words[job, row]))
print(f'PASS independent Fraction compressed-Q4 pipeline:all64 actual-RV32 result words,{commands} rounded phase predictions,8192 original synthetic weights;no CXX codec/SFPU/FPU oracle shared;NO hardware/model-quality/speed proof')
