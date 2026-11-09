#!/usr/bin/env python3
"""Cross-compile isolated dense-input capture; never build or open a driver."""
import hashlib
import os
from pathlib import Path
import subprocess
import sys
import tempfile

if len(sys.argv) != 3 or any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ):
    raise SystemExit('absolute SDK and pinned baseline dense object required')
sdk, baseline = map(Path, sys.argv[1:])
digest = 'b1d5ca2062d284533188e546a32b84a01c0280ee49246bba83830bff81decead'
if not sdk.is_absolute() or not sdk.is_dir() or not baseline.is_absolute() or baseline.is_symlink() or hashlib.sha256(baseline.read_bytes()).hexdigest() != digest:
    raise SystemExit('path or baseline identity refused')
here = Path(__file__).resolve().parent
root = here.parent
cross = sdk / 'cross-tools-riscv64/bin/riscv64-unknown-haiku-g++'
nm = sdk / 'cross-tools-riscv64/bin/riscv64-unknown-haiku-nm'
flags = '''-std=c++11 -O2 -Wall -Wextra -Werror -ffreestanding -fno-exceptions -fno-rtti
 -fno-stack-protector -fno-pic -fno-pie -ffp-contract=off -ffunction-sections -fdata-sections
 -fno-unwind-tables -fno-asynchronous-unwind-tables -msmall-data-limit=0 -mno-relax
 -march=rv32imf -mabi=ilp32 -fstack-usage -ffixed-s11'''.split()
names = '''GLOBAL_CHAIN_COMPONENT_IMAGE SIGMOID_PRECISION_IMAGE GLOBAL_CHAIN_BF16_FPU
 GLOBAL_CHAIN_HYBRID_BFP HYBRID_MATRIX_ATTENTION_INSPECTION DENSE_BF16_MATRIX_EXPERIMENT
 HEAD_MATRIX_ROOT_INSPECTION GLOBAL_CHAIN_SINGLE_CLOSED_AUDIT GLOBAL_CHAIN_WORKER_SEALED_ADMISSION
 GLOBAL_CHAIN_PRECISE_ACTIVATION GLOBAL_CHAIN_LOCAL_SOURCE_GUARDS GLOBAL_CHAIN_QUIET_UNROLL
 GLOBAL_CHAIN_BOUNDARY_SEALS GLOBAL_CHAIN_F32_HIDDEN GLOBAL_CHAIN_SEAL_UNROLL
 LAYER_RETURN_COMPONENT_IMAGE LAYER_COMPONENT_DENSE MIXED_LIVE_BLOCK_ROOT_IMAGE
 GLOBAL_CHAIN_TOKEN_KV_APPEND'''.split()
roles = ['-DPIONEER_Q38_' + n + '=1' for n in names]
capture = '-DPIONEER_Q38_QGW8_DENSE_INPUT_TRACE_IMAGE=1'
source = str(root / 'blackhole_qwen38_return_dense_layer.cpp')
with tempfile.TemporaryDirectory(prefix='qgw8-dense-input-', dir='/private/tmp') as tmp:
    out = Path(tmp)
    for variant in ('ordinary', 'input', 'stage'):
        obj = out / (variant + '.o')
        option = [capture] if variant == 'input' else (['-DPIONEER_Q38_QGW8_DENSE_STAGE_TRACE_IMAGE=1'] if variant == 'stage' else [])
        subprocess.run([str(cross), *flags, *roles, *option, '-c', source, '-o', str(obj)], check=True)
        symbols = subprocess.check_output([str(nm), '-C', str(obj)], text=True)
        assert ('TraceDenseInput(' in symbols) == (variant == 'input')
        if variant == 'ordinary':
            assert hashlib.sha256(obj.read_bytes()).hexdigest() == digest
        if variant == 'stage':
            assert hashlib.sha256(obj.read_bytes()).hexdigest() == 'd0366ecce020319d17154842052ebd65ae3239e01b843062c053600383983db0'
    for forbidden in ('PIONEER_Q38_WORK_INFERENCE', 'TT_PIONEER_Q38_WORK_MODE',
                      'PIONEER_Q38_CPU_VALIDATION', 'PIONEER_Q38_FULL_MODEL_CPU_REFERENCE',
                      '_KERNEL_MODE', 'PIONEER_Q38_QGW8_DENSE_STAGE_TRACE_IMAGE'):
        result = subprocess.run([str(cross), *flags, *roles, capture, '-D'+forbidden+'=1', '-fsyntax-only', source], capture_output=True)
        assert result.returncode and b'Dense-stage trace is restricted' in result.stderr, forbidden
    result = subprocess.run([str(cross), *flags, *roles, '-DPIONEER_Q38_QGW8_DENSE_INPUT_TRACE_IMAGE=0', '-fsyntax-only', source], capture_output=True)
    assert result.returncode and b'Dense-stage trace is restricted' in result.stderr
    for required in ('GLOBAL_CHAIN_COMPONENT_IMAGE', 'MIXED_LIVE_BLOCK_ROOT_IMAGE', 'LAYER_COMPONENT_DENSE',
                     'GLOBAL_CHAIN_F32_HIDDEN', 'GLOBAL_CHAIN_HYBRID_BFP',
                     'HYBRID_MATRIX_ATTENTION_INSPECTION', 'HEAD_MATRIX_ROOT_INSPECTION'):
        reduced = [r for r in roles if r != '-DPIONEER_Q38_' + required + '=1']
        assert subprocess.run([str(cross), *flags, *reduced, capture, '-fsyntax-only', source], capture_output=True).returncode, required
    binary = out / 'geometry'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O2', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=address,undefined', str(here / 'pioneer_q38_qgw8_dense_input_geometry_test.cpp'), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
print('PASS isolated dense-input capture cross-compile, unchanged ordinary object, geometry and role refusals; NO CARD')
