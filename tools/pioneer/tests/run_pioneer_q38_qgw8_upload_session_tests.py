#!/usr/bin/env python3
"""Test the isolated QGW8 session using synthetic sources and transport."""
import os
from pathlib import Path
import subprocess
import tempfile

here = Path(__file__).resolve().parent
tools = here.parent
cxx = os.environ.get('CXX', 'c++')
common = [cxx, '-std=c++17', '-O2', '-g', '-Wall', '-Wextra', '-Werror', '-ffp-contract=off']
required = ('PIONEER_Q38_QGW8_USER_SESSION_INSPECTION', 'PIONEER_Q38_HYBRID_UPLOAD_DIAGNOSTIC',
            'PIONEER_Q38_HYBRID_HEAD_UPLOAD_DIAGNOSTIC', 'PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION',
            'PIONEER_Q38_QGW8_LAYOUT_INSPECTION', 'PIONEER_Q38_QGW8_SPAN_INSPECTION',
            'PIONEER_Q38_QGW8_CONTROL_INSPECTION', 'PIONEER_Q38_HYBRID_TRACKED_UPLOAD')
roles = ['-D' + name + '=1' for name in required] + ['-DPIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION=1',
                                                      '-DPIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT=1']
probe = str(here / 'pioneer_q38_qgw8_upload_session_role_probe.cpp')
subprocess.run(common + roles + ['-fsyntax-only', probe], check=True)
for name in required:
    flags = [flag for flag in roles if flag != '-D' + name + '=1']
    for variant in ([], ['-D' + name + '=0']):
        result = subprocess.run(common + flags + variant + ['-fsyntax-only', probe], text=True,
                                stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        if result.returncode == 0 or 'isolated non-loadable inspection only' not in result.stderr:
            raise RuntimeError('QGW8 session required role not enforced: ' + name)
for name in ('_KERNEL_MODE', 'PIONEER_DRIVER_BUILD_TAG', 'PIONEER_Q38_TOKEN_NATIVE_CONTEXT',
             'PIONEER_Q38_FULL_OFFLOAD_NATIVE', 'PIONEER_Q38_FULL_OFFLOAD_CLIENT',
             'PIONEER_Q38_BF16_FULL_NATIVE', 'PIONEER_Q38_BF16_FULL_CLIENT',
             'PIONEER_Q38_WORK_INFERENCE', 'TT_PIONEER_Q38_WORK_MODE',
             'PIONEER_Q38_CPU_VALIDATION', 'PIONEER_Q38_FULL_MODEL_CPU_REFERENCE'):
    result = subprocess.run(common + roles + ['-D' + name, '-fsyntax-only', probe], text=True,
                            stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if result.returncode == 0 or 'isolated non-loadable inspection only' not in result.stderr:
        raise RuntimeError('QGW8 session forbidden role accepted: ' + name)
subprocess.run(common + ['-fsyntax-only', str(here / 'pioneer_q38_qgw8_upload_session_compile.cpp')], check=True)
with tempfile.TemporaryDirectory(prefix='pioneer-qgw8-upload-session-') as directory:
    binary = str(Path(directory) / 'session')
    subprocess.run(common + ['-Wno-unused-function', '-Wno-unused-variable', '-fsanitize=address,undefined',
        str(here / 'pioneer_q38_qgw8_upload_session_test.cpp'), str(tools / 'PioneerModelWeightCache.cpp'), '-o', binary], check=True)
    env = dict(os.environ, ASAN_OPTIONS='quarantine_size_mb=32', UBSAN_OPTIONS='halt_on_error=1')
    for args in ([], ['--bad-seal'], ['--strict-reply']):
        subprocess.run([binary, *args], check=True, env=env)
print('PASS isolated QGW8 session roles and full synthetic owner/receipt lifecycle; no model/device/TPS')
