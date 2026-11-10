#!/usr/bin/env python3
"""Connect QGWD pages/upload to the full-session controller in simulation."""
import os
from pathlib import Path
import subprocess
import tempfile

assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
here = Path(__file__).resolve().parent
source = str(here / 'pioneer_q38_qgwd_full_session_test.cpp')
compiler = os.environ.get('CXX', 'c++')
with tempfile.TemporaryDirectory(prefix='qgwd-full-session-', dir='/private/tmp') as tmp:
    binary = str(Path(tmp) / 'full')
    common = [compiler, '-std=c++17', '-O2', '-g', '-Wall', '-Wextra', '-Werror',
              '-Wno-misleading-indentation', '-Wno-unused-function']
    subprocess.run(common + ['-fsanitize=address,undefined', source,
                   str(here.parent / 'PioneerModelWeightCache.cpp'), '-o', binary], check=True)
    environment = dict(os.environ, ASAN_OPTIONS='quarantine_size_mb=32', UBSAN_OPTIONS='halt_on_error=1')
    for args in ([], ['--fail-start']):
        subprocess.run([binary, *args], check=True, env=environment)
    for role in ('_KERNEL_MODE', 'PIONEER_DRIVER_BUILD_TAG', 'PIONEER_Q38_TOKEN_NATIVE_CONTEXT',
                 'PIONEER_Q38_WORK_INFERENCE', 'TT_PIONEER_Q38_WORK_MODE', 'PIONEER_Q38_CPU_VALIDATION',
                 'PIONEER_Q38_QGW8_CANDIDATE_CLIENT_128', 'PIONEER_Q38_QGWC_CLIENT_136'):
        result = subprocess.run(common + ['-D'+role+'=1', '-E', source], capture_output=True)
        assert result.returncode, role
    late = subprocess.run(common + ['-fsyntax-only', str(here/'pioneer_q38_qgwd_full_late_role_test.cpp')], capture_output=True)
    assert late.returncode and b'late QGW8 protocol identity change' in late.stderr
print('PASS QGWD synthetic full-session role exclusions; NO DEVICE')
