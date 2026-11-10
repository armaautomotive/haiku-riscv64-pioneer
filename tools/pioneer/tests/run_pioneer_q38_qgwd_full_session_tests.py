#!/usr/bin/env python3
"""Connect QGWD pages/upload to the full-session controller in simulation."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile

assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
candidate_only=sys.argv[1:]==['--candidate-only']
assert not sys.argv[1:] or candidate_only
here = Path(__file__).resolve().parent
source = str(here / 'pioneer_q38_qgwd_full_session_test.cpp')
compiler = os.environ.get('CXX', 'c++')
with tempfile.TemporaryDirectory(prefix='qgwd-full-session-', dir='/private/tmp') as tmp:
    binary = str(Path(tmp) / 'full')
    common = [compiler, '-std=c++17', '-O2', '-g', '-Wall', '-Wextra', '-Werror',
              '-Wno-misleading-indentation', '-Wno-unused-function']
    environment = dict(os.environ, ASAN_OPTIONS='quarantine_size_mb=32', UBSAN_OPTIONS='halt_on_error=1')
    if not candidate_only:
        subprocess.run(common + ['-fsanitize=address,undefined', source,
                       str(here.parent / 'PioneerModelWeightCache.cpp'), '-o', binary], check=True)
        for args in ([], ['--fail-start']):
            subprocess.run([binary, *args], check=True, env=environment)
    candidate = str(Path(tmp) / 'candidate')
    subprocess.run(common + ['-DPIONEER_Q38_QGWD_CLIENT_141=1','-DPIONEER_Q38_DENSE_Q8_VERIFIED_MODEL_INSPECTION=1',
                   '-fsanitize=address,undefined', source, str(here.parent / 'PioneerModelWeightCache.cpp'),
                   '-o', candidate], check=True)
    for args in ([], ['--fail-start']):
        subprocess.run([candidate, *args], check=True, env=environment)
    multi = str(Path(tmp) / 'multi')
    subprocess.run(common + ['-DPIONEER_Q38_QGWD_MULTI_TOKEN_SIMULATION=1',
                   '-fsanitize=address,undefined', source, str(here.parent / 'PioneerModelWeightCache.cpp'),
                   '-o', multi], check=True)
    for args in ([], ['--fail-start'], ['--bad-feedback']):
        subprocess.run([multi, *args], check=True, env=environment)
    for forbidden in ('_KERNEL_MODE','PIONEER_DRIVER_BUILD_TAG','PIONEER_Q38_QGWD_CLIENT_141',
                      'PIONEER_Q38_QGWD_NATIVE_141','PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE',
                      'PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_FULL_MODEL_CPU_REFERENCE'):
        result=subprocess.run(common+['-DPIONEER_Q38_QGWD_MULTI_TOKEN_SIMULATION=1','-D'+forbidden+'=1','-E',source],capture_output=True)
        assert result.returncode,forbidden
    assert subprocess.run(common+['-DPIONEER_Q38_QGWD_MULTI_TOKEN_SIMULATION=0','-E',source],capture_output=True).returncode
    late_multi=subprocess.run(common+['-fsyntax-only',str(here/'pioneer_q38_qgwd_multi_late_role_test.cpp')],capture_output=True)
    assert late_multi.returncode and b'late multi-token simulation selection' in late_multi.stderr
    late_multi=subprocess.run(common+['-DPIONEER_Q38_TEST_MULTI_REMOVAL=1','-fsyntax-only',str(here/'pioneer_q38_qgwd_multi_late_role_test.cpp')],capture_output=True)
    assert late_multi.returncode and b'late multi-token simulation removal' in late_multi.stderr
    for role in ('_KERNEL_MODE', 'PIONEER_DRIVER_BUILD_TAG', 'PIONEER_Q38_TOKEN_NATIVE_CONTEXT',
                 'PIONEER_Q38_WORK_INFERENCE', 'TT_PIONEER_Q38_WORK_MODE', 'PIONEER_Q38_CPU_VALIDATION',
                 'PIONEER_Q38_QGW8_CANDIDATE_CLIENT_128', 'PIONEER_Q38_QGWC_CLIENT_136','PIONEER_Q38_QGWD_CLIENT_141'):
        result = subprocess.run(common + ['-D'+role+'=1', '-E', source], capture_output=True)
        assert result.returncode, role
    subprocess.run([sys.executable,str(here/'test_pioneer_q38_qgwd_client_role.py')],check=True)
    late = subprocess.run(common + ['-fsyntax-only', str(here/'pioneer_q38_qgwd_full_late_role_test.cpp')], capture_output=True)
    assert late.returncode and b'late QGW8 protocol identity change' in late.stderr
print('PASS QGWD synthetic full-session role exclusions; NO DEVICE')
