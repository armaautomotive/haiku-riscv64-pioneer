#!/usr/bin/env python3
"""OFFBOARD integer proxy vs the separate unchanged exact-rational model."""
import contextlib
import io
import os
import pathlib
import random
import runpy
import subprocess
import tempfile

if not __debug__:
    raise SystemExit('Assertions required')
here = pathlib.Path(__file__).resolve().parent
with contextlib.redirect_stdout(io.StringIO()) as prior_output:
    rational = runpy.run_path(str(here / 'pioneer_native_bfp_arithmetic_test.py'))
assert 'OTHER BFP8 HARDWARE WORDS UNOBSERVED' in prior_output.getvalue()
instruction = rational['instruction']
records = []

def case(a, b, phase, previous):
    expected = instruction(a, b, phase, previous)
    records.append(f'{phase} {previous:08x} {expected:08x} ' + ' '.join(f'{x:04x}' for x in a + b))
    return expected

for bits in (4, 8):
    for row in range(32):
        a = [rational['fixture_operand'](bits, row, k) for k in range(32)]
        b = [rational['activation'](k) for k in range(32)]
        previous = 0
        for first in (0, 16):
            for phase in ((0, 2) if bits == 4 else (0, 1, 2, 3)):
                previous = case(a[first:first + 16], b[first:first + 16], phase, previous)
        assert previous == rational['results'][bits][row][1]

rng = random.Random(0x3342464e)
exponents = (1, 2, 8, 16, 32, 64, 100, 126, 127, 128, 170, 200, 240, 253, 254)

def operand():
    if rng.randrange(17) == 0:
        return rng.randrange(2) << 15
    exponent = rng.choice(exponents) if rng.randrange(2) else rng.randrange(1, 255)
    return (rng.randrange(2) << 15) | (exponent << 7) | rng.randrange(128)

old_values = (0, 0x80000000, 0x00800000, 0x80800000, 0x3f800000, 0xbf800000,
              0x7f7fffff, 0xff7fffff, 0x7f800000, 0xff800000)
for trial in range(24000):
    a, b = [operand() for _ in range(16)], [operand() for _ in range(16)]
    if trial % 8 == 0:
        # Sign cancellation and zero lanes remain separate from range stress.
        a[8:] = a[:8]
        b[8:] = [x ^ 0x8000 for x in b[:8]]
    if trial % 11 == 0:
        a[:8] = [0] * 8
    previous = rng.choice(old_values) if trial % 2 else (rng.randrange(2) << 31) | (rng.randrange(1, 255) << 23) | rng.randrange(1 << 23)
    case(a, b, trial % 4, previous)

# Long recurrent FP32 accumulation,not just independent zero-Dst instructions.
# All Qwen widths and both explicit fidelity choices;still synthetic operands.
for width in (5120, 6144, 17408):
    for phases in ((0,), (0, 1, 2, 3)):
        for row in (0, 17):
            previous = 0
            for tile in range(width // 32):
                a = [rational['fixture_operand'](8, row, (k + tile * 7) % 32) for k in range(32)]
                b = [rational['activation']((k + tile * 11) % 32) for k in range(32)]
                for first in (0, 16):
                    for phase in phases:
                        previous = case(a[first:first + 16], b[first:first + 16], phase, previous)

temporary = pathlib.Path(tempfile.mkdtemp(prefix='pioneer-blackhole-arithmetic.'))
compiler = os.environ.get('CXX', 'c++')
subprocess.run([compiler, '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                '-fsanitize=address,undefined', str(here / 'pioneer_blackhole_bf16_arithmetic_test.cpp'),
                '-o', str(temporary / 'test')], check=True)
environment = dict(os.environ, ASAN_OPTIONS='quarantine_size_mb=32', UBSAN_OPTIONS='halt_on_error=1')
subprocess.run([str(temporary / 'test')], input='\n'.join(records) + '\n', text=True, check=True, env=environment)
for selection in ('_KERNEL_MODE', 'PIONEER_Q38_WORK_INFERENCE', 'PIONEER_Q38_CPU_VALIDATION'):
    guard = subprocess.run([compiler, '-std=c++17', '-D' + selection, '-x', 'c++', '-c', '-', '-o', str(temporary / (selection + '.o'))],
                           input='#include "' + str(here.parent / 'PioneerBlackholeBf16Arithmetic.h') + '"\n', text=True, capture_output=True)
    assert guard.returncode and 'Unqualified Blackhole arithmetic characterization is offboard only' in guard.stderr
print('PASS arithmetic build guards:3 forbidden runtime selections;original physical mismatch/comparators UNCHANGED')
print('OFFBOARD arithmetic evidence:', temporary)
