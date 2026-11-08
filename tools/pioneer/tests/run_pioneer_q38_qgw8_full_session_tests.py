#!/usr/bin/env python3
"""Check the QGW8 full-session path with synthetic pages and driver only."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile

assert sys.argv[1:] in ([], ['--roles-only'])
here = Path(__file__).resolve().parent
root = here.parent
compiler = os.environ.get('CXX', 'c++')
assert not any(key.startswith(('PIONEER_', 'TT_PIONEER_')) for key in os.environ)
header = root / 'PioneerQ38HybridFullSession.h'
source = '#include "' + str(header) + '"\n'
names = ('PIONEER_Q38_QGW8_FULL_SESSION_INSPECTION', 'PIONEER_Q38_QGW8_USER_SESSION_INSPECTION',
         'PIONEER_Q38_HYBRID_UPLOAD_DIAGNOSTIC', 'PIONEER_Q38_HYBRID_HEAD_UPLOAD_DIAGNOSTIC',
         'PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION', 'PIONEER_Q38_QGW8_LAYOUT_INSPECTION',
         'PIONEER_Q38_QGW8_SPAN_INSPECTION', 'PIONEER_Q38_QGW8_CONTROL_INSPECTION',
         'PIONEER_Q38_HYBRID_TRACKED_UPLOAD', 'PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION',
         'PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT')
roles = ['-D' + name + '=1' for name in names] + ['-DPIONEER_Q38_GDN_MUTABLE_STATE=1',
         '-DPIONEER_BANKS_MOCK_ONLY=1', '-DB_DEVICE_OP_CODES_END=1000']
common = [compiler, '-std=c++17', '-Wall', '-Wextra', '-Werror']
def probe(flags):
    return subprocess.run(common + flags + ['-fsyntax-only', '-x', 'c++', '-'], input=source,
                          stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
assert probe(roles).returncode == 0
for name in names:
    assert probe([flag for flag in roles if flag != '-D' + name + '=1']).returncode, name
for name in ('_KERNEL_MODE', '__FAST_MATH__', 'PIONEER_Q38_CPU_VALIDATION', 'PIONEER_Q38_FULL_MODEL_CPU_REFERENCE',
             'PIONEER_Q38_WORK_INFERENCE', 'TT_PIONEER_Q38_WORK_MODE', 'PIONEER_Q38_FULL_OFFLOAD_CLIENT',
             'PIONEER_Q38_FULL_OFFLOAD_NATIVE', 'PIONEER_Q38_BF16_FULL_CLIENT', 'PIONEER_Q38_BF16_FULL_NATIVE',
             'PIONEER_DRIVER_BUILD_TAG', 'PIONEER_Q38_TOKEN_NATIVE_CONTEXT', 'PIONEER_Q38_HYBRID_FULL_NATIVE',
             'PIONEER_Q38_HYBRID_MATRIX_NATIVE_127', 'PIONEER_Q38_QGW8_FULL_NATIVE_INSPECTION',
             'PIONEER_Q38_HYBRID_FULL_CLIENT',
             'PIONEER_Q38_HYBRID_CANDIDATE_UPLOAD_CLIENT', 'PIONEER_Q38_HYBRID_TRACKED_UPLOAD_CLIENT',
             'PIONEER_Q38_HYBRID_FIXED_126', 'PIONEER_Q38_HYBRID_MATRIX_CLIENT_127'):
    assert probe(roles + ['-D' + name + '=1']).returncode, name
assert probe([flag for flag in roles if flag != '-DPIONEER_Q38_QGW8_FULL_SESSION_INSPECTION=1']
             + ['-DPIONEER_Q38_QGW8_FULL_SESSION_INSPECTION=0']).returncode
late = source + ('#undef PIONEER_Q38_QGW8_FULL_SESSION_INSPECTION\n'
                 '#define PIONEER_Q38_HYBRID_FULL_CLIENT 1\n'
                 '#define PIONEER_Q38_HYBRID_CANDIDATE_UPLOAD_CLIENT 1\n'
                 '#define PIONEER_Q38_HYBRID_TRACKED_UPLOAD_CLIENT 1\n') + source
result = subprocess.run(common + roles + ['-fsyntax-only', '-x', 'c++', '-'], input=late,
                        stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
assert result.returncode and 'late legacy full-session role change' in result.stderr
upload_header = root / 'PioneerQ38Qgw8UploadSessionInspection.h'
late_upload = ('#include "' + str(upload_header) + '"\n'
               '#define PIONEER_Q38_QGW8_FULL_SESSION_INSPECTION 1\n') + source
upload_roles = [flag for flag in roles if flag != '-DPIONEER_Q38_QGW8_FULL_SESSION_INSPECTION=1']
result = subprocess.run(common + upload_roles + ['-fsyntax-only', '-x', 'c++', '-'], input=late_upload,
                        stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
assert result.returncode and 'late QGW8 full-session selector' in result.stderr
if sys.argv[1:] == ['--roles-only']:
    print('PASS QGW8 full-session role guards; no device')
    raise SystemExit(0)
with tempfile.TemporaryDirectory(prefix='pioneer-qgw8-full-session-') as name:
    binary = str(Path(name) / 'test')
    subprocess.run(common + ['-O2', '-g', '-ffp-contract=off', '-Wno-unused-function', '-Wno-unused-variable',
        '-fsanitize=address,undefined', str(here / 'pioneer_q38_qgw8_full_session_test.cpp'),
        str(root / 'PioneerModelWeightCache.cpp'), '-o', binary], check=True)
    env = dict(os.environ, ASAN_OPTIONS='quarantine_size_mb=32', UBSAN_OPTIONS='halt_on_error=1')
    for args in (['--wrong-tag'], ['--bad-support'], [], ['--fail-start'], ['--bad-record']):
        subprocess.run([binary, *args], check=True, env=env)
print('PASS QGW8 session role guards and synthetic same-FD token lifecycle; no driver, device, model or TPS')
