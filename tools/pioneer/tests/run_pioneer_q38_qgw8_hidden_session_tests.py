#!/usr/bin/env python3
"""Tag129 hidden client and protocol over synthetic pages/transport only."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile

assert sys.argv[1:] in ([], ['--roles-only'])
assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
here = Path(__file__).resolve().parent
root = here.parent
compiler = os.environ.get('CXX', 'c++')
header = root / 'PioneerQ38HybridFullSession.h'
source = '#include "' + str(header) + '"\nstatic_assert(PioneerQ38FullOffload::BuildTag==0x2026110700010129ULL);\n'
names = ('PIONEER_Q38_QGW8_FULL_SESSION_INSPECTION', 'PIONEER_Q38_QGW8_USER_SESSION_INSPECTION',
         'PIONEER_Q38_HYBRID_UPLOAD_DIAGNOSTIC', 'PIONEER_Q38_HYBRID_HEAD_UPLOAD_DIAGNOSTIC',
         'PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION', 'PIONEER_Q38_QGW8_LAYOUT_INSPECTION',
         'PIONEER_Q38_QGW8_SPAN_INSPECTION', 'PIONEER_Q38_QGW8_CONTROL_INSPECTION',
         'PIONEER_Q38_HYBRID_TRACKED_UPLOAD', 'PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION',
         'PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT', 'PIONEER_Q38_QGW8_CANDIDATE_CLIENT_128',
         'PIONEER_Q38_QGW8_HIDDEN_CLIENT_129')
roles = ['-D' + n + '=1' for n in names] + ['-DPIONEER_Q38_GDN_MUTABLE_STATE=1',
         '-DPIONEER_BANKS_MOCK_ONLY=1', '-DB_DEVICE_OP_CODES_END=1000']
common = [compiler, '-std=c++17', '-Wall', '-Wextra', '-Werror']
def probe(flags, text=source):
    return subprocess.run(common + flags + ['-fsyntax-only', '-x', 'c++', '-'], input=text,
                          stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
assert probe(roles).returncode == 0
for name in names:
    assert probe([flag for flag in roles if flag != '-D' + name + '=1']).returncode, name
for name in ('_KERNEL_MODE', '__FAST_MATH__', 'PIONEER_Q38_CPU_VALIDATION', 'PIONEER_Q38_FULL_MODEL_CPU_REFERENCE',
             'PIONEER_Q38_WORK_INFERENCE', 'TT_PIONEER_Q38_WORK_MODE', 'PIONEER_Q38_FULL_OFFLOAD_NATIVE',
             'PIONEER_Q38_QGW8_CANDIDATE_NATIVE_128', 'PIONEER_Q38_QGW8_HIDDEN_NATIVE_129',
             'PIONEER_Q38_QGW8_COARSE_PROGRAMS_INSPECTION'):
    assert probe(roles + ['-D' + name + '=1']).returncode, name
bad = [f for f in roles if f != '-DPIONEER_Q38_QGW8_HIDDEN_CLIENT_129=1'] + ['-DPIONEER_Q38_QGW8_HIDDEN_CLIENT_129=0']
assert probe(bad).returncode
late = source + '#undef PIONEER_Q38_QGW8_HIDDEN_CLIENT_129\n#include "' + str(header) + '"\n'
assert probe(roles, late).returncode
if sys.argv[1:] == ['--roles-only']:
    print('PASS tag129 hidden client role checks; no device')
    raise SystemExit(0)
with tempfile.TemporaryDirectory(prefix='pioneer-qgw8-hidden-session-') as name:
    binary = str(Path(name) / 'test')
    subprocess.run(common + ['-O2', '-g', '-ffp-contract=off', '-Wno-unused-function', '-Wno-unused-variable',
        '-fsanitize=address,undefined', str(here / 'pioneer_q38_qgw8_hidden_session_test.cpp'),
        str(root / 'PioneerModelWeightCache.cpp'), '-o', binary], check=True)
    env = dict(os.environ, ASAN_OPTIONS='quarantine_size_mb=32', UBSAN_OPTIONS='halt_on_error=1')
    for args in (['--wrong-input'], ['--wrong-tag'], [], ['--fail-first'], ['--fail-second'],
                 ['--bad-reply'], ['--changed-request'], ['--fail-finish']):
        subprocess.run([binary, *args], check=True, env=env)
print('PASS tag129 USER fixed input/owner/drain/read/finish/close and fault cases; synthetic only')
