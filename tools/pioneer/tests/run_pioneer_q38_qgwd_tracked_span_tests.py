#!/usr/bin/env python3
"""Exercise actual QGWD receipt + span service with pinned RV32 simulations."""
import hashlib
import os
from pathlib import Path
import subprocess
import sys
import tempfile

assert __debug__ and not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
if len(sys.argv) != 3:
    raise SystemExit('usage: run_pioneer_q38_qgwd_tracked_span_tests.py ABS_STRICT_IMAGE ABS_FAST_REUSE_IMAGE')
strict, fast = map(Path, sys.argv[1:])
pins = (('10cc657069152a0f8b216490105fee287a517b680cc6af04f3d598906a448b61', strict),
        ('9b4e950f3d016d229b7f9ed7c5dd9f2e0fc23a2865a5f1a36f6633216f397609', fast))
boot = 'e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b'
for pin, image in pins:
    assert image.is_absolute() and image.is_dir()
    assert hashlib.sha256((image / 'text.bin').read_bytes()).hexdigest() == pin
    assert hashlib.sha256((image / 'boot.bin').read_bytes()).hexdigest() == boot
here = Path(__file__).resolve().parent
source = str(here / 'pioneer_q38_qgwd_tracked_span_test.cpp')
compiler = os.environ.get('CXX', 'c++')
common = [compiler, '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror', '-ffp-contract=off',
          '-DTT_PIONEER_EXPERIMENTAL_Q38_BANKS']
with tempfile.TemporaryDirectory(prefix='qgwd-span-service-', dir='/private/tmp') as tmp:
    for name, image, flags in (('strict', strict, []), ('fast', fast, ['-DPIONEER_Q38_SPAN_FAST=1',
                               '-DPIONEER_TEST_QGWD_FAST=1', '-DPIONEER_Q38_SPAN_REUSE_CODE=1'])):
        binary = str(Path(tmp) / name)
        subprocess.run(common + flags + ['-fsanitize=address,undefined', source, '-o', binary], check=True)
        subprocess.run([binary, str(image)], check=True,
                       env=dict(os.environ, ASAN_OPTIONS='quarantine_size_mb=32', UBSAN_OPTIONS='halt_on_error=1'))
    for role in ('_KERNEL_MODE', 'PIONEER_DRIVER_BUILD_TAG', 'PIONEER_Q38_TOKEN_NATIVE_CONTEXT',
                 'PIONEER_Q38_WORK_INFERENCE', 'TT_PIONEER_Q38_WORK_MODE', 'PIONEER_Q38_CPU_VALIDATION'):
        result = subprocess.run(common + ['-D'+role+'=1', '-E', source], capture_output=True)
        assert result.returncode, role
    wrong = subprocess.run(common + ['-DPIONEER_TEST_QGWD_WRONG_PAIR=1', '-fsyntax-only', source], capture_output=True)
    assert wrong.returncode and b'Unsupported tracked span receipt/Work pairing' in wrong.stderr
print('PASS strict/fast QGWD span service and role exclusions; SIMULATION ONLY')
