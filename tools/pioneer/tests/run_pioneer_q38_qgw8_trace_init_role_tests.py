#!/usr/bin/env python3
"""Compile-only role isolation checks; no card or driver access."""
import pathlib
import subprocess

root = pathlib.Path(__file__).resolve().parents[3]
role = 'tools/pioneer/PioneerQ38Qgw8TraceInitRole.h'
required = '''PIONEER_Q38_QGW8_TRACE_INIT_INSPECTION PIONEER_Q38_QGW8_TRACE_READ_INSPECTION
_KERNEL_MODE PIONEER_Q38_QGW8_NATIVE_INSPECTION PIONEER_Q38_HYBRID_NATIVE_COMPILE_ONLY
PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY PIONEER_Q38_TOKEN_NATIVE_CONTEXT PIONEER_Q38_HYBRID_TRACKED_UPLOAD
PIONEER_Q38_QGW8_LAYOUT_INSPECTION PIONEER_Q38_QGW8_PROGRAMS_INSPECTION PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION
PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT'''.split()
symbols = 'api_version init_hardware init_driver uninit_driver publish_devices find_device'.split()
good = {name: '1' for name in required}
good.update({name: 'renamed_' + name for name in symbols})
good['PIONEER_DRIVER_BUILD_TAG'] = '0x51544b4e4f445256ULL'

def compiles(macros, tail=''):
    cmd = ['c++', '-std=c++17', '-fsyntax-only', '-x', 'c++', '-I', str(root)]
    cmd += ['-D' + name + '=' + value for name, value in macros.items()]
    source = '#include "' + role + '"\n' + tail
    return subprocess.run(cmd + ['-'], input=source, text=True, capture_output=True).returncode == 0

assert compiles(good)
cases = 0
for name in required[1:] + symbols + ['PIONEER_DRIVER_BUILD_TAG']:
    missing = dict(good); del missing[name]
    assert not compiles(missing), name
    cases += 1
for name in required:
    wrong = dict(good); wrong[name] = '0'
    assert not compiles(wrong), name
    cases += 1
for name in '''PIONEER_Q38_QGW8_TRACE_INIT_DATA_INSPECTION PIONEER_Q38_QGW8_CANDIDATE_NATIVE_128
PIONEER_Q38_QGW8_HIDDEN_NATIVE_129 PIONEER_Q38_QGW8_LAYER_TRACE_IMAGE
PIONEER_Q38_QGW8_LAYER_TRACE_PROGRAMS_INSPECTION PIONEER_Q38_HYBRID_FULL_NATIVE
PIONEER_Q38_FULL_OFFLOAD_NATIVE PIONEER_Q38_FULL_OFFLOAD_CLIENT PIONEER_Q38_WORK_INFERENCE
TT_PIONEER_Q38_WORK_MODE PIONEER_Q38_CPU_VALIDATION PIONEER_Q38_FULL_MODEL_CPU_REFERENCE'''.split():
    for value in ('0', '1'):
        wrong = dict(good); wrong[name] = value
        assert not compiles(wrong), name
        cases += 1
assert not compiles(good, '#undef PIONEER_Q38_QGW8_TRACE_INIT_INSPECTION\n#include "' + role + '"\n')
plain = dict(good); del plain['PIONEER_Q38_QGW8_TRACE_INIT_INSPECTION']
assert not compiles(plain, '#define PIONEER_Q38_QGW8_TRACE_INIT_INSPECTION 1\n#include "' + role + '"\n')
print('PASS fake trace init role:', cases, 'missing/wrong/conflicting refusals and two late-binding refusals; NO DEVICE')
