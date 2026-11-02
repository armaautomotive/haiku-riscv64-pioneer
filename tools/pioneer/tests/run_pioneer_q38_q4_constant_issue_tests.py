#!/usr/bin/env python3
"""Pure emitter/selection tests only; never card IO."""
import os
import pathlib
import subprocess
import tempfile

assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
here = pathlib.Path(__file__).resolve().parent
flags = [
    '-DPIONEER_Q38_Q4_CONSTANT_ISSUE_IMAGE=1', '-DPIONEER_Q38_Q4_CONSTANT_ISSUE=1',
    '-DPIONEER_Q38_Q4_CONSTANT_ISSUE_PARENT_BINDING=1',
    '-DPIONEER_Q38_TOKEN_TWIN_BATCH_PREFETCH_WORKER_IMAGE=1', '-DPIONEER_Q38_WORK_TWIN_BATCH_PREFETCH=1',
    '-DPIONEER_Q38_STATIC_VECTOR', '-DPIONEER_Q38_REPLAY_OPERAND',
    '-DPIONEER_Q38_NATIVE_OPERAND_STORE', '-DPIONEER_Q38_VECTOR_INLINE',
]
with tempfile.TemporaryDirectory(prefix='pioneer-q4-constant-emitter-', dir='/private/tmp') as tmp:
    binary = pathlib.Path(tmp)/'test'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=address,undefined', *flags, str(here/'pioneer_q38_q4_constant_issue_test.cpp'),
                    '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True, env={**os.environ, 'UBSAN_OPTIONS':'halt_on_error=1'})
    source = '#include "PioneerQ38Q4ConstantIssueRole.h"\n'
    base = [os.environ.get('CXX', 'c++'), '-std=c++17', '-fsyntax-only', '-x', 'c++', '-I', str(here.parent)]
    positives = 0
    refusals = 0
    for selected in ([], flags):
        subprocess.run([*base, *selected, '-'], input=source, text=True, check=True)
        positives += 1
    variants = []
    for i, flag in enumerate(flags):
        variants.append(flags[:i]+flags[i+1:])
        if flag.endswith('=1'):
            variants.extend(flags[:i]+[flag[:-1]+str(value)]+flags[i+1:] for value in (0,2))
    for macro in ('_KERNEL_MODE', 'PIONEER_DRIVER_BUILD_TAG', '__FAST_MATH__',
                  'PIONEER_Q38_CPU_VALIDATION', 'PIONEER_Q38_FULL_MODEL_CPU_REFERENCE',
                  'PIONEER_Q38_WORK_INFERENCE', 'TT_PIONEER_Q38_WORK_MODE',
                  'PIONEER_Q38_WORK_RAW_UNPACK_CACHE', 'PIONEER_Q38_WORK_Q4_NIBBLE',
                  'PIONEER_Q38_WORK_Q4_DIRECT_DECODE', 'PIONEER_Q38_WORK_FRONTEND_BARRIER',
                  'PIONEER_Q38_NATIVE_Q8', 'PIONEER_Q38_PREDECODED_Q4',
                  'PIONEER_Q38_TOKEN_NATIVE_CONTEXT', 'PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE'):
        variants.extend([*flags, '-D'+macro+'='+str(value)] for value in (0,1))
    for variant in variants:
        result = subprocess.run([*base, *variant, '-'], input=source, text=True, capture_output=True)
        assert result.returncode and 'error:' in result.stderr, variant
        refusals += 1
    # Header is deliberately rechecked even when its enum has been bound.
    for change in ('#undef PIONEER_Q38_Q4_CONSTANT_ISSUE_IMAGE\n',
                   '#undef PIONEER_Q38_Q4_CONSTANT_ISSUE\n',
                   '#define PIONEER_Q38_CPU_VALIDATION 0\n'):
        result = subprocess.run([*base, *flags, '-'], input=source+change+source, text=True, capture_output=True)
        assert result.returncode and 'error:' in result.stderr
        refusals += 1
    print('PASS_CONSTANT_Q4_ROLES positives'+str(positives)+' refusals'+str(refusals)+';BUILD only, no native selector')
