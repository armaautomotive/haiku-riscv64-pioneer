#!/usr/bin/env python3
"""Offline padded recurrent BFP8 placement and role isolation."""
import os
from pathlib import Path
import subprocess
import tempfile

here = Path(__file__).resolve().parent
source = here / 'pioneer_q38_small_recurrent_bfp8_plan_test.cpp'
header = here.parent / 'PioneerQ38HybridSmallRecurrentBfp8Plan.h'
compiler = os.environ.get('CXX', 'c++')
assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
with tempfile.TemporaryDirectory(prefix='q38-small-layout-', dir='/private/tmp') as tmp:
    binary = str(Path(tmp) / 'test')
    subprocess.run([compiler, '-std=c++17', '-O2', '-g', '-Wall', '-Wextra', '-Werror',
                    '-Wno-misleading-indentation', '-Wno-unused-function', '-fsanitize=address,undefined',
                    str(source), '-o', binary], check=True)
    subprocess.run([binary], check=True, env={**os.environ, 'UBSAN_OPTIONS': 'halt_on_error=1'})
    for forbidden in ('_KERNEL_MODE', 'PIONEER_DRIVER_BUILD_TAG', 'PIONEER_Q38_TOKEN_NATIVE_CONTEXT',
                      'PIONEER_Q38_WORK_INFERENCE', 'TT_PIONEER_Q38_WORK_MODE',
                      'PIONEER_Q38_CPU_VALIDATION', 'PIONEER_Q38_FULL_MODEL_CPU_REFERENCE'):
        result = subprocess.run([compiler, '-std=c++17', '-D' + forbidden + '=1', '-E', str(source)], capture_output=True)
        assert result.returncode and b'Small recurrent BFP8 plan is offline geometry only' in result.stderr, forbidden
    late = subprocess.run([compiler, '-std=c++17', '-fsyntax-only', '-x', 'c++', '-'],
                          input='#include "' + str(source) + '"\n#undef PIONEER_Q38_SMALL_RECURRENT_BFP8_PLAN_INSPECTION\n'
                                '#include "' + str(header) + '"\n', text=True, capture_output=True)
    assert late.returncode and 'Small recurrent BFP8 plan is offline geometry only' in late.stderr
print('PASS padded recurrent BFP8 plan rejects native, Work and reference roles; NO DEVICE')
