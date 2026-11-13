#!/usr/bin/env python3
"""Simulated prepared-span transport and receipt, never a hardware test."""
import os
from pathlib import Path
import subprocess
import tempfile

here = Path(__file__).resolve().parent
source = here / 'pioneer_q38_small_recurrent_span_upload_test.cpp'
compiler = os.environ.get('CXX', 'c++')
assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
with tempfile.TemporaryDirectory(prefix='q38-small-span-transport-', dir='/private/tmp') as tmp:
    binary = str(Path(tmp) / 'test')
    subprocess.run([compiler, '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror', '-Wno-unused-function',
                    '-Wno-misleading-indentation', '-fsanitize=address,undefined', str(source), '-o', binary], check=True)
    subprocess.run([binary], check=True, env={**os.environ, 'ASAN_OPTIONS': 'quarantine_size_mb=32', 'UBSAN_OPTIONS': 'halt_on_error=1'})
    for name in ('_KERNEL_MODE', 'PIONEER_DRIVER_BUILD_TAG', 'PIONEER_Q38_TOKEN_NATIVE_CONTEXT',
                 'PIONEER_Q38_WORK_INFERENCE', 'TT_PIONEER_Q38_WORK_MODE',
                 'PIONEER_Q38_FULL_OFFLOAD_CLIENT', 'PIONEER_Q38_FULL_OFFLOAD_NATIVE',
                 'PIONEER_Q38_BF16_FULL_CLIENT', 'PIONEER_Q38_BF16_FULL_NATIVE',
                 'PIONEER_Q38_CPU_VALIDATION', 'PIONEER_Q38_FULL_MODEL_CPU_REFERENCE'):
        result = subprocess.run([compiler, '-std=c++17', '-D' + name + '=1', '-E', str(source)], capture_output=True)
        assert result.returncode and b'isolated inspection' in result.stderr, name
print('PASS recurrent prepared-span transport role isolation; NO DEVICE')
