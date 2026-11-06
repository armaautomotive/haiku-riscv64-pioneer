#!/usr/bin/env python3
"""Preprocess the actual TILE build selector,not an unrelated header probe."""
import os
from pathlib import Path
import subprocess
import sys

assert __debug__ and not os.environ.get('PYTHONOPTIMIZE') and len(sys.argv) == 2
sdk = Path(sys.argv[1]); assert sdk.is_absolute() and sdk.is_dir()
source = Path(__file__).resolve().parents[1] / 'blackhole_q38_q4_factored_tile.cpp'
cc = str(sdk / 'cross-tools-riscv64/bin/riscv64-unknown-haiku-g++')
base = [cc, '-std=c++17', '-march=rv32i', '-mabi=ilp32', '-ffreestanding',
        '-DPIONEER_Q38_Q4_FACTORED_BFP8_BUILD=1']
def run(flags):
    return subprocess.run(base + flags + ['-E', str(source)], stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
assert run(['-DPIONEER_Q38_Q4_FACTORED_TILE_BUILD=1']).returncode == 0
refused = 0
for flags in ([], ['-DPIONEER_Q38_Q4_FACTORED_TILE_BUILD=0'], ['-DPIONEER_Q38_Q4_FACTORED_TILE_BUILD=2']):
    r = run(flags); assert r.returncode and b'Exact separate factored-tile' in r.stderr; refused += 1
for role in ('_KERNEL_MODE', 'PIONEER_Q38_WORK_INFERENCE', 'TT_PIONEER_Q38_WORK_MODE',
             'PIONEER_Q38_CPU_VALIDATION', 'PIONEER_Q38_FULL_MODEL_CPU_REFERENCE',
             'PIONEER_Q38_FULL_OFFLOAD_NATIVE', 'PIONEER_Q38_FULL_OFFLOAD_CLIENT',
             'PIONEER_DRIVER_BUILD_TAG', 'TT_PIONEER_DRIVER_RELOAD', '__FAST_MATH__'):
    for value in (0, 1):
        r = run(['-DPIONEER_Q38_Q4_FACTORED_TILE_BUILD=1', f'-D{role}={value}'])
        assert r.returncode and b'NO native,Work,client,reference or driver integration' in r.stderr
        refused += 1
assert refused == 23
print(f'PASS_FACTORED_TILE_ROLES actualRV32Source positive1 refusals{refused};BUILD_ONLY_NOT_NATIVE_AUTHORITY')
