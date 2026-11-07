#!/usr/bin/env python3
"""Check QGW8 page-to-span integration on synthetic sources/fabric only."""
import os
from pathlib import Path
import subprocess
import tempfile

here = Path(__file__).resolve().parent
tools = here.parent
cxx = os.environ.get('CXX', 'c++')
common = [cxx, '-std=c++17', '-O2', '-g', '-Wall', '-Wextra', '-Werror', '-ffp-contract=off']
probe = str(here / 'pioneer_q38_hybrid_head_span_role_probe.cpp')
head = ['-DPIONEER_Q38_HYBRID_HEAD_UPLOAD_DIAGNOSTIC=1', '-DPIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION=1',
        '-DPIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION=1', '-DPIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT=1']
subprocess.run(common + ['-fsyntax-only', probe], check=True)
subprocess.run(common + head + ['-fsyntax-only', probe], check=True)
for flags, reason in [
        (['-DPIONEER_Q38_HYBRID_HEAD_UPLOAD_DIAGNOSTIC=1'], 'distinct head-plan inspection role'),
        (['-DPIONEER_Q38_HYBRID_HEAD_UPLOAD_DIAGNOSTIC=0'], 'distinct head-plan inspection role'),
        (head + ['-DPIONEER_TEST_UNSUPPORTED_PLAN'], 'Only explicit hybrid upload plans'),
        (['-DPIONEER_TEST_UNSUPPORTED_PLAN'], 'Only the legacy hybrid upload plan')]:
    result = subprocess.run(common + flags + ['-fsyntax-only', probe], text=True,
                            stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if result.returncode == 0 or reason not in result.stderr:
        raise RuntimeError('Upload selection guard failed: ' + repr(flags))
for role in ('_KERNEL_MODE', 'PIONEER_Q38_WORK_INFERENCE', 'TT_PIONEER_Q38_WORK_MODE',
             'PIONEER_Q38_FULL_OFFLOAD_CLIENT', 'PIONEER_Q38_CPU_VALIDATION'):
    result = subprocess.run(common + head + ['-D' + role, '-fsyntax-only', probe], text=True,
                            stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if result.returncode == 0 or 'separate experimental diagnostic' not in result.stderr:
        raise RuntimeError('Upload role guard failed: ' + role)
with tempfile.TemporaryDirectory(prefix='pioneer-qgw8-head-span-') as tmp:
    binary = str(Path(tmp) / 'bridge')
    subprocess.run(common + ['-Wno-unused-function', '-fsanitize=address,undefined',
        str(here / 'pioneer_q38_hybrid_head_span_bridge_test.cpp'),
        str(tools / 'PioneerModelWeightCache.cpp'), '-o', binary], check=True)
    env = dict(os.environ, ASAN_OPTIONS='quarantine_size_mb=32', UBSAN_OPTIONS='halt_on_error=1')
    subprocess.run([binary], check=True, env=env)
print('PASS QGW8 explicit layout, role isolation, 3551 synthetic parts and complete span packet stream; no model/device/TPS')
