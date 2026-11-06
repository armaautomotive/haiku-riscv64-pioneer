#!/usr/bin/env python3
"""Configured component remains separate from Work/native authority."""
import os
from pathlib import Path
import subprocess
import sys

assert __debug__ and len(sys.argv) == 2 and not os.environ.get('PYTHONOPTIMIZE')
here = Path(__file__).resolve().parent
sdk = Path(sys.argv[1]); assert sdk.is_absolute() and sdk.is_dir()
header = here.parent / 'PioneerQ38Q4FactoredConfiguredCompute.h'
body = '#include "' + str(header) + '"\n'
cc = [os.environ.get('CXX', 'c++'), '-std=c++17', '-x', 'c++', '-fsyntax-only']
positive = subprocess.run([*cc, '-DPIONEER_Q38_Q4_FACTORED_BFP8_OFFBOARD=1', '-'], input=body,
                          text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
assert positive.returncode == 0 and not positive.stdout and not positive.stderr
cases = [[f'-D{role}={value}'] for role in
         ('_KERNEL_MODE', 'PIONEER_Q38_WORK_INFERENCE', 'TT_PIONEER_Q38_WORK_MODE',
          'PIONEER_Q38_CPU_VALIDATION', 'PIONEER_Q38_FULL_MODEL_CPU_REFERENCE',
          'PIONEER_Q38_FULL_OFFLOAD_NATIVE', 'PIONEER_Q38_FULL_OFFLOAD_CLIENT',
          'PIONEER_DRIVER_BUILD_TAG', 'TT_PIONEER_DRIVER_RELOAD') for value in (0, 1)]
cases.append(['-ffast-math'])
for extra in cases:
    result = subprocess.run([*cc, '-DPIONEER_Q38_Q4_FACTORED_BFP8_OFFBOARD=1', *extra, '-'],
                            input=body, text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    assert result.returncode != 0 and 'error:' in result.stderr, extra
cross = str(sdk / 'cross-tools-riscv64/bin/riscv64-unknown-haiku-g++')
for value in (0, 2):
    result = subprocess.run([cross, '-std=c++17', '-march=rv32i', '-mabi=ilp32', '-fsyntax-only',
                            '-DPIONEER_Q38_FACTORED_Q4_COMPARE_BUILD=1',
                            '-DPIONEER_Q38_Q4_FACTORED_BFP8_BUILD=1',
                            '-DPIONEER_Q38_FACTORED_CONFIGURED_COMPUTE=' + str(value),
                            str(here.parent / 'blackhole_q38_factored_q4_compare.cpp')],
                            stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    assert result.returncode != 0 and b'Exact separate configured-compute' in result.stderr
print('PASS_CONFIGURED_ROLES positive1 refused' + str(len(cases) + 2) +
      '_WorkNativeDriverReferenceAliases_and_badSelectors;COMPONENT_ONLY_NO_DRIVER_AUTHORITY')
