#!/usr/bin/env python3
"""Check the actual returning component source,not an unrelated header."""
import os
from pathlib import Path
import subprocess
import sys

assert __debug__ and not os.environ.get('PYTHONOPTIMIZE') and len(sys.argv) == 2
sdk = Path(sys.argv[1])
assert sdk.is_absolute() and sdk.is_dir()
source = Path(__file__).resolve().parents[1] / 'blackhole_q38_factored_q4_matrix.cpp'
cc = str(sdk / 'cross-tools-riscv64/bin/riscv64-unknown-haiku-g++')
base = [cc, '-std=c++17', '-march=rv32i', '-mabi=ilp32',
        '-DPIONEER_Q38_FACTORED_Q4_MATRIX_BUILD=1', '-DPIONEER_Q38_Q4_FACTORED_BFP8_BUILD=1']
def run(extra):
    return subprocess.run(base + extra + ['-E', str(source)], stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
assert run([]).returncode == 0
assert run(['-DPIONEER_Q38_FACTORED_Q4_RETURN_ONLY=1']).returncode == 0
cases = []
for role in ('PIONEER_Q38_FACTORED_Q4_MATRIX_BUILD', 'PIONEER_Q38_Q4_FACTORED_BFP8_BUILD'):
    for value in (None, 0, 2):
        cases.append(['-U' + role] + ([] if value is None else ['-D' + role + '=' + str(value)]))
for value in (0, 2):
    cases.append(['-DPIONEER_Q38_FACTORED_Q4_RETURN_ONLY=' + str(value)])
for role in ('_KERNEL_MODE', 'PIONEER_Q38_WORK_INFERENCE', 'TT_PIONEER_Q38_WORK_MODE',
             'PIONEER_Q38_CPU_VALIDATION', 'PIONEER_Q38_FULL_MODEL_CPU_REFERENCE',
             'PIONEER_Q38_FULL_OFFLOAD_NATIVE', 'PIONEER_Q38_FULL_OFFLOAD_CLIENT',
             'PIONEER_DRIVER_BUILD_TAG', 'TT_PIONEER_DRIVER_RELOAD', '__FAST_MATH__',
             'PIONEER_Q38_CACHED_FPU_MATRIX_IMAGE', 'PIONEER_Q38_CACHED_FPU_PAIR_ISSUE_IMAGE',
             'PIONEER_Q38_BF16_PRODUCER_IMAGE', 'PIONEER_Q38_Q4_FACTORED_TILE_BUILD',
             'PIONEER_Q38_COMPACT_Q4_FPU_STREAM_IMAGE'):
    for value in (0, 1):
        cases.append(['-D' + role + '=' + str(value)])
cases.append(['-march=rv64imac', '-mabi=lp64'])
for extra in cases:
    result = run(extra)
    assert result.returncode and b'error:' in result.stderr, extra
assert len(cases) == 39
print('PASS_FACTORED_RETURNING_ROLES actualSource positive2 refusals39;BUILD_ONLY_NOT_NATIVE_AUTHORITY')
