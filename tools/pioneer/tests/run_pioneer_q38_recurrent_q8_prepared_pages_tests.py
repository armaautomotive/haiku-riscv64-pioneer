#!/usr/bin/env python3
"""Synthetic recurrent BFP8 cache/page routing, never a model or device."""
import os
from pathlib import Path
import subprocess
import tempfile

here = Path(__file__).resolve().parent
root = here.parent
source = here / 'pioneer_q38_recurrent_q8_prepared_pages_test.cpp'
assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
compiler = os.environ.get('CXX', 'c++')
with tempfile.TemporaryDirectory(prefix='q38-recurrent-q8-pages-', dir='/private/tmp') as directory:
    binary = str(Path(directory) / 'test')
    subprocess.run([compiler, '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                    '-Wno-misleading-indentation', '-Wno-unused-function', '-Wno-unused-variable',
                    '-fsanitize=address,undefined', str(source), str(root/'PioneerModelWeightCache.cpp'),
                    '-o', binary], check=True)
    subprocess.run([binary], check=True, env=dict(os.environ, UBSAN_OPTIONS='halt_on_error=1'))
    for forbidden in ('_KERNEL_MODE', 'PIONEER_DRIVER_BUILD_TAG', 'PIONEER_Q38_TOKEN_NATIVE_CONTEXT',
                      'PIONEER_Q38_WORK_INFERENCE', 'TT_PIONEER_Q38_WORK_MODE',
                      'PIONEER_Q38_CPU_VALIDATION', 'PIONEER_Q38_FULL_MODEL_CPU_REFERENCE'):
        result = subprocess.run([compiler, '-std=c++17', '-D'+forbidden+'=1', '-E', str(source)], capture_output=True)
        assert result.returncode and b'Recurrent Q8 prepared pages are isolated offline data only' in result.stderr, forbidden
print('PASS recurrent prepared-page native/Work/reference exclusions; NO DEVICE')
