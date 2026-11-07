#!/usr/bin/env python3
"""SDK compile/refusal checks; build isolation only, never device authority."""
import os
from pathlib import Path
import subprocess
import sys

assert __debug__ and len(sys.argv) == 2
assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
here = Path(__file__).resolve().parent.parent
build = Path(sys.argv[1])
assert build.is_absolute()
header = '#include "PioneerQ38EarlyJobSnapshotBuildRole.h"\n'
selector = 'PIONEER_Q38_EARLY_JOB_SNAPSHOT_IMAGE'
required = ('PIONEER_Q38_BATCHED_OUTPUT_CLOSURE_ROOT_IMAGE', 'PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE',
            'PIONEER_Q38_LAYER_RETURN_COMPONENT_IMAGE', 'PIONEER_Q38_GLOBAL_CHAIN_PUSHED_COMPLETION',
            'PIONEER_Q38_GLOBAL_CHAIN_PRECISE_ACTIVATION', 'PIONEER_Q38_GLOBAL_CHAIN_LOCAL_SOURCE_GUARDS')
forbidden = ('_KERNEL_MODE', 'PIONEER_DRIVER_BUILD_TAG', 'TT_PIONEER_DRIVER_RELOAD', '__FAST_MATH__',
             'PIONEER_Q38_CPU_VALIDATION', 'PIONEER_Q38_FULL_MODEL_CPU_REFERENCE', 'PIONEER_Q38_WORK_INFERENCE',
             'TT_PIONEER_Q38_WORK_MODE', 'PIONEER_Q38_FULL_OFFLOAD_NATIVE', 'PIONEER_Q38_FULL_OFFLOAD_CLIENT',
             'PIONEER_Q38_TOKEN_NATIVE_CONTEXT', 'PIONEER_Q38_TOKEN_PUSHED_NATIVE_CONTEXT',
             'PIONEER_Q38_GLOBAL_WORKER_IMAGE', 'PIONEER_Q38_GLOBAL_CHAIN_ROOT_IMAGE',
             'PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE', 'PIONEER_Q38_FAST_DECODE_PROFILE_IMAGE',
             'PIONEER_Q38_BF16_PROFILE_IMAGE', 'PIONEER_Q38_GLOBAL_CHAIN_BF16_FPU',
             'PIONEER_Q38_PREPARED_Q4_ROOT_IMAGE', 'PIONEER_Q38_ROOT_COUNTER_UNROLL_IMAGE')
positive = refused = 0

def name(flag):
    return flag[2:].split('=', 1)[0]

for kind in ('recurrent', 'dense'):
    command = (build / ('compile-' + kind + '-argv.txt')).read_text().splitlines()
    defines = [f for f in command if f.startswith('-D')]
    assert any(name(f) == selector for f in defines)
    base = [command[0], '-std=c++11', '-march=rv32imf', '-mabi=ilp32', '-Wall', '-Wextra', '-Werror',
            '-fsyntax-only', '-x', 'c++', '-I', str(here)]
    def check(flags, okay, source=header):
        global positive, refused
        result = subprocess.run([*base, *flags, '-'], input=source, capture_output=True, text=True)
        assert (result.returncode == 0) == okay, (kind, flags, result.stderr)
        if okay:
            assert not result.stderr
            positive += 1
        else:
            refused += 1
    check(defines, True)
    initial = [f for f in defines if name(f) != selector]
    check(initial, True)
    for value in ('0', '2'):
        check(initial + ['-D' + selector + '=' + value], False)
    family = ('PIONEER_Q38_LAYER_COMPONENT_RECURRENT', 'PIONEER_Q38_MIXED_RECURRENT_BLOCK_ROOT_IMAGE') if kind == 'recurrent' else (
        'PIONEER_Q38_LAYER_COMPONENT_DENSE', 'PIONEER_Q38_MIXED_LIVE_BLOCK_ROOT_IMAGE')
    for macro in required + family:
        assert any(name(f) == macro for f in defines)
        check([f for f in defines if name(f) != macro], False)
        check([f for f in defines if name(f) != macro] + ['-D' + macro + '=0'], False)
    for macro in forbidden:
        for value in ('0', '1'):
            check(defines + ['-D' + macro + '=' + value], False)
    other = 'PIONEER_Q38_LAYER_COMPONENT_DENSE' if kind == 'recurrent' else 'PIONEER_Q38_LAYER_COMPONENT_RECURRENT'
    check(defines + ['-D' + other + '=1'], False)
    check(defines, False, header + '#undef ' + selector + '\n' + header)
    check(initial, False, header + '#define ' + selector + ' 1\n' + header)
print(f'PASS snapshot role: {positive} accepted, {refused} rejected; actual SDK; NO_DEVICE')
