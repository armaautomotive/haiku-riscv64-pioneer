#!/usr/bin/env python3
"""Compile-only CPU-product separation, including defined-zero/late mixing."""
import os
import pathlib
import subprocess
here = pathlib.Path(__file__).resolve().parent
assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
compiler = os.environ.get('CXX', 'c++')
header = str(here/'pioneer_q38_cpu_model_reference.h')
include = f'#include "{header}"\n'
base = '#define PIONEER_Q38_FULL_MODEL_CPU_REFERENCE 1\n#define PIONEER_Q38_CPU_VALIDATION 1\n'
def check(source, okay, diagnostic=''):
    result = subprocess.run([compiler, '-std=c++17', '-Wall', '-Wextra', '-Werror', '-x', 'c++', '-fsyntax-only', '-'], input=source, text=True, capture_output=True)
    assert (result.returncode == 0) == okay and (okay or diagnostic in result.stderr), result.stderr
check(base+include+'int main(){return 0;}\n', True)
cases = 1
for parent in ('PIONEER_Q38_FULL_MODEL_CPU_REFERENCE', 'PIONEER_Q38_CPU_VALIDATION'):
    for value in (None, 0, 2):
        source = ''.join(f'#define {p} {1 if p != parent else value}\n' for p in ('PIONEER_Q38_FULL_MODEL_CPU_REFERENCE', 'PIONEER_Q38_CPU_VALIDATION') if p != parent or value is not None)
        check(source+include, False, 'Exact separate whole-model CPU observer role required')
        cases += 1
for role in ('_KERNEL_MODE', '__FAST_MATH__', 'PIONEER_ATTENTION_FREESTANDING', 'PIONEER_Q38_WORK_INFERENCE', 'TT_PIONEER_Q38_WORK_MODE',
             'PIONEER_Q38_NATIVE_Q8_LAYOUT', 'PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE', 'PIONEER_Q38_GLOBAL_WORKER_IMAGE',
             'PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE', 'PIONEER_Q38_GLOBAL_CHAIN_F32_HIDDEN', 'PIONEER_Q38_TOKEN_CANCEL_BOUNDARY',
             'PIONEER_Q38_RECURRENT_PREFIX_NATIVE_CONTEXT', 'PIONEER_Q38_RECURRENT_PREFIX_VALIDATION_NATIVE',
             'PIONEER_Q38_RECURRENT_BLOCK_NATIVE_CONTEXT', 'PIONEER_Q38_MIXED_LIVE_NATIVE_CONTEXT', 'PIONEER_Q38_EMBEDDING_NATIVE',
             'PIONEER_Q38_LEARNED_RMS_NATIVE', 'PIONEER_Q38_DECODER_RUNTIME', 'PIONEER_Q38_MIXED_WORKER_IMAGE',
             'PIONEER_Q38_MIXED_RECURRENT_BLOCK_ROOT_IMAGE', 'PIONEER_Q38_MIXED_LIVE_BLOCK_ROOT_IMAGE', 'PIONEER_Q38_GLOBAL_CHAIN_ROOT_IMAGE',
             'PIONEER_Q38_TOKEN_NATIVE_CONTEXT', 'PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY',
             'PIONEER_Q38_TOKEN_NATIVE_FULL_FLOW_FIXTURE', 'PIONEER_Q38_TOKEN_NATIVE_FULL_END_FLOW_FIXTURE',
             'PIONEER_Q38_NATIVE_STATE_TEST_BRIDGE', 'PIONEER_Q38_TOKEN_NATIVE_STATE_SNAPSHOT_FIXTURE'):
    for value in (0, 1):
        for late in (False, True):
            definition = f'#define {role} {value}\n'
            source = base+(include if late else '')+definition+include
            check(source, False, 'CPU whole-model products must NEVER enter')
            cases += 1
print(f'PASS {cases} CPU-reference compile gates/exact parents/kernel+image+native+Work/defined0+1 and late reinclusion;not numerical or hardware qualification')
