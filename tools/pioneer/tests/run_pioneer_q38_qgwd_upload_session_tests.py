#!/usr/bin/env python3
"""Exercise the distinct synthetic QGWD upload session; never access a device."""
import os
from pathlib import Path
import subprocess
import tempfile

assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
here = Path(__file__).resolve().parent
source = here / 'pioneer_q38_qgwd_upload_session_test.cpp'
compiler = os.environ.get('CXX', 'c++')
with tempfile.TemporaryDirectory(prefix='qgwd-upload-session-', dir='/private/tmp') as tmp:
    binary = Path(tmp) / 'session'
    subprocess.run([compiler, '-std=c++17', '-O2', '-g', '-Wall', '-Wextra', '-Werror',
                    '-Wno-misleading-indentation', '-Wno-unused-function', '-fsanitize=address,undefined',
                    str(source), str(here.parent / 'PioneerModelWeightCache.cpp'), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True,
                   env=dict(os.environ, ASAN_OPTIONS='quarantine_size_mb=32', UBSAN_OPTIONS='halt_on_error=1'))
    for role in ('_KERNEL_MODE', 'PIONEER_DRIVER_BUILD_TAG', 'PIONEER_Q38_WORK_INFERENCE',
                 'TT_PIONEER_Q38_WORK_MODE', 'PIONEER_Q38_CPU_VALIDATION',
                 'PIONEER_Q38_QGW8_FULL_SESSION_INSPECTION', 'PIONEER_Q38_QGW8_CANDIDATE_CLIENT_128'):
        result = subprocess.run([compiler, '-std=c++17', '-D'+role+'=1', '-E', str(source)], capture_output=True)
        assert result.returncode, role
print('PASS QGWD isolated-session role exclusions; NO DEVICE')
