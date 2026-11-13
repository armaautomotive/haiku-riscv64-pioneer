#!/usr/bin/env python3
"""Private recurrent export role and refusal tests; never open hardware."""
import os
from pathlib import Path
import subprocess
import tempfile

assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
here = Path(__file__).resolve().parent
source = here.parent / 'pioneer_native_model_band_export.cpp'
cache = here.parent / 'PioneerModelWeightCache.cpp'
compiler = os.environ.get('CXX', 'c++')
common = [compiler, '-std=c++17', '-O2', '-Wall', '-Wextra', '-Werror',
          '-Wno-misleading-indentation', '-ffp-contract=off']
roles = ('PIONEER_NATIVE_RECURRENT_BAND_EXPORT','PIONEER_NATIVE_RECURRENT_OUT_BAND_EXPORT','PIONEER_NATIVE_RECURRENT_QKV_BAND_EXPORT')
for name in roles:
    role='-D'+name+'=1'
    for forbidden in ('_KERNEL_MODE', 'PIONEER_DRIVER_BUILD_TAG', 'PIONEER_Q38_TOKEN_NATIVE_CONTEXT',
                      'PIONEER_Q38_WORK_INFERENCE', 'TT_PIONEER_Q38_WORK_MODE',
                      'PIONEER_Q38_CPU_VALIDATION', 'PIONEER_Q38_FULL_MODEL_CPU_REFERENCE', '__FAST_MATH__',
                      'PIONEER_NATIVE_QGWD_DENSE_BAND_EXPORT', *(other for other in roles if other!=name)):
        result = subprocess.run([*common, role, '-D' + forbidden + '=1', '-fsyntax-only', str(source)], capture_output=True)
        assert result.returncode, (name,forbidden)
with tempfile.TemporaryDirectory(prefix='recurrent-band-export-test-', dir='/private/tmp') as directory:
    for name in roles:
        binary = str(Path(directory) / name)
        subprocess.run([*common, '-D'+name+'=1', str(source), str(cache), '-o', binary], check=True)
        assert subprocess.run([binary], capture_output=True).returncode == 2
        result = subprocess.run([binary, '/dev/null', directory, directory], capture_output=True)
        assert result.returncode == 1 and b'DO_NOT_COMPILE_PARTIAL_OUTPUT' in result.stderr
        assert not (Path(directory) / 'PioneerNativeBfpModelBandData.h').exists()
print('PASS private recurrent export refuses invalid model and native/Work/reference/dense roles; NO DEVICE')
