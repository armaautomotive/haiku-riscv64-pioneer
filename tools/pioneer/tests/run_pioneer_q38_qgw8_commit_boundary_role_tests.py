#!/usr/bin/env python3
"""QGW8 commit optimization build-role guards only; no card or model."""
import os
from pathlib import Path
import subprocess

assert not any(key.startswith(('PIONEER_', 'TT_PIONEER_')) for key in os.environ)
role = Path(__file__).resolve().parent.parent / 'PioneerQ38Qgw8CommitBoundaryRole.h'
selector = 'PIONEER_Q38_QGW8_COMMIT_BOUNDARY_IMAGE'
required = (selector, '__riscv', 'PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE',
            'PIONEER_Q38_GLOBAL_CHAIN_FEEDBACK', 'PIONEER_Q38_TOKEN_CANCEL_BOUNDARY',
            'PIONEER_Q38_FULL_OFFLOAD_ROOT_IMAGE', 'PIONEER_Q38_GLOBAL_CHAIN_BF16_FPU',
            'PIONEER_Q38_GLOBAL_CHAIN_HYBRID_BFP', 'PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION',
            'PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT', 'PIONEER_Q38_HEAD_MATRIX_ROOT_INSPECTION')
forbidden = ('PIONEER_Q38_TOKEN_COMMIT_BOUNDARY_SEALS_IMAGE', 'PIONEER_Q38_TOKEN_PHASE_PROFILE_IMAGE',
             '_KERNEL_MODE', 'PIONEER_Q38_TOKEN_NATIVE_CONTEXT', 'PIONEER_Q38_GLOBAL_WORKER_IMAGE',
             'PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE', 'PIONEER_Q38_CPU_VALIDATION',
             'PIONEER_Q38_FULL_MODEL_CPU_REFERENCE', 'PIONEER_Q38_WORK_INFERENCE',
             'TT_PIONEER_Q38_WORK_MODE', '__FAST_MATH__')
base = dict.fromkeys(required, '1')
include = '#include "' + str(role) + '"\n'
checks = 0

def check(values, okay, tail=''):
    global checks
    source = ''.join('#define ' + key + ' ' + value + '\n' for key, value in sorted(values.items())) + include + tail
    result = subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++11', '-Werror', '-fsyntax-only', '-x', 'c++', '-'],
                            input=source, text=True, capture_output=True)
    assert (result.returncode == 0) == okay, (values, tail, result.stderr)
    checks += 1

check({}, True, 'static_assert(PioneerQ38Qgw8CommitBoundaryProfile==0,"default");\n')
check(base, True, include + 'static_assert(PioneerQ38Qgw8CommitBoundaryProfile==1,"isolated");\n')
for key in required:
    for value in (None, '0', '2'):
        bad = base.copy()
        if value is None:
            del bad[key]
        else:
            bad[key] = value
        if key != selector or value is not None:
            check(bad, False)
    check(base, False, '#undef ' + key + '\n#define ' + key + ' 0\n' + include)
for key in forbidden:
    for value in ('0', '1'):
        check({**base, key: value}, False)
check({}, False, '#define ' + selector + ' 1\n' + include)
check(base, False, '#undef ' + selector + '\n' + include)
print(f'PASS QGW8 commit ROOT role {checks} syntax checks; NO driver, model, hardware, Work or TPS')
