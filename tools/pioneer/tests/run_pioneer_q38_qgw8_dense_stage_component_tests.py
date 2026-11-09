#!/usr/bin/env python3
"""Compile isolated dense-stage capture; no device or driver activation."""
import hashlib
import os
from pathlib import Path
import subprocess
import sys
import tempfile

if len(sys.argv) != 3 or any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ):
    raise SystemExit('absolute SDK and pinned baseline dense object required')
sdk, baseline = map(Path, sys.argv[1:])
if not sdk.is_absolute() or not baseline.is_absolute() or not baseline.is_file() or baseline.is_symlink():
    raise SystemExit('invalid paths')
expected = 'b1d5ca2062d284533188e546a32b84a01c0280ee49246bba83830bff81decead'
if hashlib.sha256(baseline.read_bytes()).hexdigest() != expected:
    raise SystemExit('baseline identity refused')
here = Path(__file__).resolve().parent
root = here.parent
prefix = str(sdk / 'cross-tools-riscv64/bin/riscv64-unknown-haiku-')
flags = ['-std=c++11', '-O2', '-Wall', '-Wextra', '-Werror', '-ffreestanding',
         '-fno-exceptions', '-fno-rtti', '-fno-stack-protector', '-fno-pic', '-fno-pie',
         '-ffp-contract=off', '-ffunction-sections', '-fdata-sections', '-fno-unwind-tables',
         '-fno-asynchronous-unwind-tables', '-msmall-data-limit=0', '-mno-relax',
         '-march=rv32imf', '-mabi=ilp32', '-fstack-usage', '-ffixed-s11']
names = '''GLOBAL_CHAIN_COMPONENT_IMAGE SIGMOID_PRECISION_IMAGE GLOBAL_CHAIN_BF16_FPU
 GLOBAL_CHAIN_HYBRID_BFP HYBRID_MATRIX_ATTENTION_INSPECTION DENSE_BF16_MATRIX_EXPERIMENT
 HEAD_MATRIX_ROOT_INSPECTION GLOBAL_CHAIN_SINGLE_CLOSED_AUDIT GLOBAL_CHAIN_WORKER_SEALED_ADMISSION
 GLOBAL_CHAIN_PRECISE_ACTIVATION GLOBAL_CHAIN_LOCAL_SOURCE_GUARDS GLOBAL_CHAIN_QUIET_UNROLL
 GLOBAL_CHAIN_BOUNDARY_SEALS GLOBAL_CHAIN_F32_HIDDEN GLOBAL_CHAIN_SEAL_UNROLL
 LAYER_RETURN_COMPONENT_IMAGE LAYER_COMPONENT_DENSE MIXED_LIVE_BLOCK_ROOT_IMAGE
 GLOBAL_CHAIN_TOKEN_KV_APPEND'''.split()
roles = ['-DPIONEER_Q38_' + name + '=1' for name in names]
capture = '-DPIONEER_Q38_QGW8_DENSE_STAGE_TRACE_IMAGE=1'
source = str(root / 'blackhole_qwen38_return_dense_layer.cpp')
with tempfile.TemporaryDirectory(prefix='pioneer-qgw8-dense-stage-', dir='/private/tmp') as tmp:
    out = Path(tmp)
    for enabled in (False, True):
        target = out / ('trace.o' if enabled else 'ordinary.o')
        subprocess.run([prefix + 'g++', *flags, *roles, *([capture] if enabled else []),
                        '-c', source, '-o', str(target)], check=True)
        symbols = subprocess.check_output([prefix + 'nm', '-C', str(target)], text=True)
        if ('TraceDenseStage(' in symbols) != enabled:
            raise RuntimeError('experimental symbol selection failed')
        if not enabled and hashlib.sha256(target.read_bytes()).hexdigest() != expected:
            raise RuntimeError('ordinary dense component changed')
    for forbidden in ('PIONEER_Q38_WORK_INFERENCE', 'TT_PIONEER_Q38_WORK_MODE',
                      'PIONEER_Q38_CPU_VALIDATION', 'PIONEER_Q38_FULL_MODEL_CPU_REFERENCE', '_KERNEL_MODE'):
        result = subprocess.run([prefix + 'g++', *flags, *roles, capture, '-D' + forbidden + '=1',
                                 '-fsyntax-only', source], capture_output=True, text=True)
        if result.returncode == 0 or 'Dense-stage trace is restricted' not in result.stderr:
            raise RuntimeError('forbidden role accepted: ' + forbidden)
    for required in ('GLOBAL_CHAIN_COMPONENT_IMAGE', 'MIXED_LIVE_BLOCK_ROOT_IMAGE', 'LAYER_COMPONENT_DENSE',
                     'GLOBAL_CHAIN_F32_HIDDEN', 'GLOBAL_CHAIN_HYBRID_BFP',
                     'HYBRID_MATRIX_ATTENTION_INSPECTION', 'HEAD_MATRIX_ROOT_INSPECTION'):
        reduced = [x for x in roles if x != '-DPIONEER_Q38_' + required + '=1']
        result = subprocess.run([prefix + 'g++', *flags, *reduced, capture, '-fsyntax-only', source],
                                capture_output=True, text=True)
        if result.returncode == 0:
            raise RuntimeError('missing required role accepted: ' + required)
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O2', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=address,undefined', str(here / 'pioneer_q38_qgw8_dense_stage_trace_geometry_test.cpp'),
                    '-o', str(out / 'geometry')], check=True)
    subprocess.run([str(out / 'geometry')], check=True)
print('PASS separate dense trace cross-compile, ordinary binary identity and role refusals; NO LINKED IMAGE, CARD OR WORK')
