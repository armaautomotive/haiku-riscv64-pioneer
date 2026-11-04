#!/usr/bin/env python3
"""Independent Fraction check of CLOSED synthetic owner-test outputs;NO GPU."""
import contextlib
from fractions import Fraction
import io
import pathlib
import re
import runpy
import sys
if not __debug__ or len(sys.argv) != 2:
    raise SystemExit('Use non-optimized Python and one CLOSED owner-test log')
here = pathlib.Path(__file__).resolve().parent
with pathlib.Path(sys.argv[1]).open('rb') as stream:
    raw = stream.read(1024 * 1024 + 1)
assert len(raw) <= 1024 * 1024
text = raw.decode('utf-8')
words = {}
for line in text.splitlines():
    if not line.startswith('BAND_LEASE_SYNTHETIC_REFERENCE'):
        continue
    match = re.fullmatch(r'BAND_LEASE_SYNTHETIC_REFERENCE k(5120|6144|17408) row([0-9]+) word([0-9a-f]{8})', line)
    assert match
    width, row = map(int, match.group(1, 2))
    assert row < 32 and (width, row) not in words
    words[width, row] = int(match.group(3), 16)
assert set(words) == {(width, row) for width in (5120, 6144, 17408) for row in range(32)}
for width in (5120, 6144, 17408):
    assert text.count(f'PASS OFFBOARD full-K owner K{width} ') == 1
assert text.count('PASS OFFBOARD full-K owner:21 failure/no-retry/close cases;') == 1
assert text.count('PASS full-K owner build guards:driver/Work/validation refused;') == 1
with contextlib.redirect_stdout(io.StringIO()):
    rational = runpy.run_path(str(here / 'pioneer_native_bfp_arithmetic_test.py'))
# This independent dyadic arithmetic consumes neither C++ PackTile, fixture,
# integer-Instruction implementation, shader observer nor saved owner memory.
encoded = {n: rational['encode_normal'](Fraction(n, 128)) >> 16 for n in range(-127, 128)}
instructions = 0
for width in (5120, 6144, 17408):
    for row in range(32):
        total = 0
        for tile in range(width // 32):
            a = [encoded[((row * 37 + c * 19 + tile * 11) % 255) - 127] for c in range(32)]
            b = [0x3f00 + ((c * 13 + tile * 7) % 256) + (0x8000 if c & 3 == 2 else 0) for c in range(32)]
            for half in (0, 16):
                for phase in range(4):
                    total = rational['instruction'](a[half:half + 16], b[half:half + 16], phase, total)
                    instructions += 1
        assert words[width, row] == total, (width, row, hex(words[width, row]), hex(total))
    print(f'PASS independent Fraction owner arithmetic K{width}:all32 predictions match closed synthetic actual-RV32 owner test;NO_PHYSICAL_OR_MODEL_SPEED_PROOF', flush=True)
print(f'PASS independent Fraction full-K owner:96 words,{instructions} phase instructions;NO log-provenance/device/driver/Work authorization')
