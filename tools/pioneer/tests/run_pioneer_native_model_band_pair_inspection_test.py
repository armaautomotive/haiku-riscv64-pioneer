#!/usr/bin/env python3
"""Offline ownership test; the private learned-band export is never published."""
import hashlib
import os
import pathlib
import subprocess
import sys
import tempfile

if not __debug__ or len(sys.argv) != 2:
    raise SystemExit('usage: python3 run_pioneer_native_model_band_pair_inspection_test.py PRIVATE_QKV_EXPORT')
private = pathlib.Path(sys.argv[1])
if not private.is_absolute() or private.is_symlink():
    raise SystemExit('expected an absolute, non-symlink private export directory')
expected = {
    'PioneerNativeBfpModelBandData.h': '396fac17c3a837d7b440fb2982e4748e8c06c6c7fbc8299acdade184ec268a8a',
    'PioneerNativeBfpModelBandMetadata.h': 'a81f5268d942344b4c4cfd4d0d02853dc82f36bfe8ab099a5bfa5d47d94de86d',
}
for name, digest in expected.items():
    path = private / name
    if path.is_symlink() or hashlib.sha256(path.read_bytes()).hexdigest() != digest:
        raise SystemExit('private QKV export does not match pinned identity')
here = pathlib.Path(__file__).resolve().parent
cxx = os.environ.get('CXX', 'c++')
with tempfile.TemporaryDirectory(prefix='pioneer-band-pair-test-') as tmp:
    common = [cxx, '-std=c++17', '-I' + str(private)]
    subprocess.run(common + ['-O1', '-g', '-Wall', '-Wextra', '-Werror', '-Wno-misleading-indentation',
        '-fsanitize=address,undefined', str(here / 'pioneer_native_model_band_pair_inspection_test.cpp'),
        '-o', str(pathlib.Path(tmp) / 'test')], check=True)
    subprocess.run([str(pathlib.Path(tmp) / 'test')], check=True,
        env=dict(os.environ, ASAN_OPTIONS='quarantine_size_mb=32', UBSAN_OPTIONS='halt_on_error=1'))
    header = '#include "' + str(here.parent / 'PioneerNativeModelBandPairInspection.h') + '"\n'
    for forbidden in ('_KERNEL_MODE', 'PIONEER_DRIVER_BUILD_TAG', 'PIONEER_NATIVE_MODEL_BAND_DIAGNOSTIC',
                      'PIONEER_Q38_TOKEN_NATIVE_CONTEXT', 'TT_PIONEER_Q38_WORK_MODE',
                      'PIONEER_Q38_WORK_INFERENCE', 'PIONEER_Q38_CPU_VALIDATION',
                      'PIONEER_Q38_FULL_MODEL_CPU_REFERENCE', '__FAST_MATH__'):
        result = subprocess.run(common + ['-DPIONEER_NATIVE_MODEL_BAND_PAIR_INSPECTION=1', '-D' + forbidden + '=1',
            '-fsyntax-only', '-x', 'c++', '-'], input=header, text=True, capture_output=True)
        if result.returncode == 0 or 'Two-core learned-band owner is offline inspection only' not in result.stderr:
            raise SystemExit('missing role refusal: ' + forbidden)
    result = subprocess.run(common + ['-fsyntax-only', '-x', 'c++', '-'], input=header, text=True, capture_output=True)
    if result.returncode == 0 or 'Two-core learned-band owner is offline inspection only' not in result.stderr:
        raise SystemExit('missing inspection-selection refusal')
for name, digest in expected.items():
    if hashlib.sha256((private / name).read_bytes()).hexdigest() != digest:
        raise SystemExit('private export changed during test')
print('PASS_OFFLINE_PAIR_ROLE_REFUSAL; NO_KERNEL_DRIVER_OR_HARDWARE_EXECUTION')
