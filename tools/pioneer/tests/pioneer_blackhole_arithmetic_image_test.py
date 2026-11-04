#!/usr/bin/env python3
"""Build actual RV32 images;separate arithmetic observer vs Fraction predictions."""
import contextlib
from fractions import Fraction
import io
import os
import pathlib
import runpy
import subprocess
import sys
import tempfile

if not __debug__ or len(sys.argv) != 2:
    raise SystemExit('Use non-optimized Python and the absolute Haiku SDK path')
sdk = pathlib.Path(sys.argv[1])
assert sdk.is_absolute() and sdk.is_dir()
here = pathlib.Path(__file__).resolve().parent
with contextlib.redirect_stdout(io.StringIO()):
    rational = runpy.run_path(str(here / 'pioneer_native_bfp_arithmetic_test.py'))
temporary = pathlib.Path(tempfile.mkdtemp(prefix='pioneer-blackhole-arithmetic-images.'))
environment = dict(os.environ, ASAN_OPTIONS='quarantine_size_mb=32', UBSAN_OPTIONS='halt_on_error=1')
for name in ('single', 'band'):
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                    '-ffp-contract=off', '-fsanitize=address,undefined', str(here / ('pioneer_blackhole_arithmetic_' + name + '_image_test.cpp')),
                    '-o', str(temporary / name)], check=True)
for bits in (4, 8):
    output = temporary / ('single' + str(bits))
    subprocess.run([sys.executable, str(here.parent / 'build_q38_native_bfp_matvec.py'), str(sdk), str(output), str(bits)], check=True)
    expected = [pair[1] for pair in rational['results'][bits]]
    subprocess.run([str(temporary / 'single'), str(output), str(bits)], input=' '.join(f'{x:08x}' for x in expected) + '\n',
                   text=True, env=environment, check=True)

# These exact n/128 synthetic weights have no native BFP8 quantization loss.
# Independent dyadic->BF16 mapping,not a call to C++ Pack/Unpack or the observer.
encoded = {n: rational['encode_normal'](Fraction(n, 128)) >> 16 for n in range(-127, 128)}
for width in (5120, 6144, 17408):
    references = {1: [], 4: []}
    for row in range(32):
        totals = {1: 0, 4: 0}
        for tile in range(width // 32):
            raw = [((row * 37 + c * 19 + tile * 11) % 255) - 127 for c in range(32)]
            assert all(64 <= max(abs(x) for x in raw[half:half + 16]) <= 127 for half in (0, 16))
            a = [encoded[n] for n in raw]
            b = [0x3f00 + ((c * 13 + tile * 7) % 256) + (0x8000 if c & 3 == 2 else 0) for c in range(32)]
            for half in (0, 16):
                for phases in (1, 4):
                    for phase in range(phases):
                        totals[phases] = rational['instruction'](a[half:half + 16], b[half:half + 16], phase, totals[phases])
        for phases in (1, 4):
            references[phases].append(totals[phases])
    for phases in (1, 4):
        for issue in ('mmio', 'direct'):
            output = temporary / f'k{width}-p{phases}-{issue}'
            command = [sys.executable, str(here.parent / 'build_native_bfp_band.py'), str(sdk), str(output), str(width), str(phases)]
            if issue == 'direct':
                command.append('--direct')
            subprocess.run(command, check=True)
            subprocess.run([str(temporary / 'band'), str(output), str(width), str(phases), issue],
                           input=' '.join(f'{x:08x}' for x in references[phases]) + '\n', text=True, env=environment, check=True)
print('PASS separate arithmetic actual RV32:2 single-tile+12 full-K/fidelity/issue variants,14 positive image executions/28 band math jobs plus9 fault cases;ORIGINAL IDEAL COMPARATORS UNCHANGED,NO HARDWARE_ACCEPTANCE')
print('OFFBOARD actual RV32 arithmetic evidence:', temporary)
