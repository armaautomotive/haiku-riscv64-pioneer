#!/usr/bin/env python3
"""Recorder data-role gates,not an execution or numerical qualifier."""
import os
import pathlib
import subprocess
here = pathlib.Path(__file__).resolve().parent
assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
compiler = os.environ.get('CXX', 'c++')
include = f'#include "{here/"PioneerQ38ObservedTrace.h"}"\n'
base = '#define PIONEER_Q38_TEST_OBSERVED_TRACE 1\n'
def check(code, valid, text=''):
    result = subprocess.run([compiler, '-std=c++17', '-Wall', '-Wextra', '-Werror', '-x', 'c++', '-fsyntax-only', '-'], input=code, text=True, capture_output=True)
    assert (result.returncode == 0) == valid and (valid or text in result.stderr), result.stderr
check(base+include, True)
cases = 1
for value in (None, 0, 2):
    check(('' if value is None else f'#define PIONEER_Q38_TEST_OBSERVED_TRACE {value}\n')+include, False, 'Exact HOST observed-output recorder role required')
    cases += 1
for role in ('_KERNEL_MODE', 'PIONEER_Q38_WORK_INFERENCE', 'PIONEER_ATTENTION_FREESTANDING', 'PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE',
             'PIONEER_Q38_GLOBAL_WORKER_IMAGE', 'PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE', 'PIONEER_Q38_RECURRENT_PREFIX_NATIVE_CONTEXT',
             'PIONEER_Q38_MIXED_LIVE_NATIVE_CONTEXT', 'PIONEER_Q38_FULL_MODEL_CPU_REFERENCE'):
    for value in (0, 1):
        for late in (False, True):
            check(base+(include if late else '')+f'#define {role} {value}\n'+include, False, 'Observed-output TEST recorder is not')
            cases += 1
for value in (0, 1):
    check(base+f'#define TT_PIONEER_Q38_WORK_MODE {value}\n'+include, True)
    cases += 1
print(f'PASS {cases} exact recorder DATA role/kernel+image+native+CPU-oracle+real Work/defined0+1+late refusals;TT_WORK arithmetic policy permitted ONLY as offline host data,not native authority')
