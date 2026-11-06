#!/usr/bin/env python3
"""Compile only: keep the new arithmetic out of all existing execution roles."""
import os
from pathlib import Path
import subprocess

assert __debug__ and not os.environ.get('PYTHONOPTIMIZE')
header = Path(__file__).resolve().parents[1] / 'PioneerQ38Q4FactoredBfp8.h'
base = ['c++', '-std=c++17', '-Werror', '-x', 'c++', '-fsyntax-only', '-']
source = '#include "' + str(header) + '"\n'
def run(flags, text=source):
    return subprocess.run(base + flags, input=text, text=True, capture_output=True)
assert run(['-DPIONEER_Q38_Q4_FACTORED_BFP8_OFFBOARD=1']).returncode == 0
refused = 0
for flags in ([], ['-DPIONEER_Q38_Q4_FACTORED_BFP8_OFFBOARD=0'], ['-DPIONEER_Q38_Q4_FACTORED_BFP8_OFFBOARD=2']):
    r = run(flags)
    assert r.returncode and 'Separate factored-Q4' in r.stderr
    refused += 1
for role in ('_KERNEL_MODE', 'PIONEER_Q38_WORK_INFERENCE', 'TT_PIONEER_Q38_WORK_MODE',
             'PIONEER_Q38_CPU_VALIDATION', 'PIONEER_Q38_FULL_MODEL_CPU_REFERENCE',
             'PIONEER_Q38_FULL_OFFLOAD_NATIVE', 'PIONEER_Q38_FULL_OFFLOAD_CLIENT',
             'PIONEER_DRIVER_BUILD_TAG', 'TT_PIONEER_DRIVER_RELOAD', '__FAST_MATH__'):
    for value in (0, 1):
        r = run(['-DPIONEER_Q38_Q4_FACTORED_BFP8_OFFBOARD=1', f'-D{role}={value}'])
        assert r.returncode and 'NO native,Work,client,reference or driver integration' in r.stderr
        refused += 1
    late = source + f'#define {role} 1\n' + source
    r = run(['-DPIONEER_Q38_Q4_FACTORED_BFP8_OFFBOARD=1'], late)
    assert r.returncode and 'NO native,Work,client,reference or driver integration' in r.stderr
    refused += 1
for value in (0, 1, 2):
    r = run(['-DPIONEER_Q38_Q4_FACTORED_BFP8_OFFBOARD=1', f'-DPIONEER_Q38_Q4_FACTORED_BFP8_BUILD={value}'])
    assert r.returncode and 'BUILD selection requires exact RV32 role' in r.stderr
    refused += 1
r = run(['-DPIONEER_Q38_Q4_FACTORED_BFP8_OFFBOARD=1', '-DPIONEER_ATTENTION_FREESTANDING=1'])
assert r.returncode and 'distinct RV32 BUILD-only role' in r.stderr
refused += 1
assert refused == 37
print(f'PASS_FACTORED_Q4_ROLES positive1 refusals{refused} including late inclusion;OFFBOARD_ONLY_NO_NATIVE_GRANT')
