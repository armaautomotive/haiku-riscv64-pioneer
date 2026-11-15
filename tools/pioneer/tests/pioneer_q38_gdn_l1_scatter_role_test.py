#!/usr/bin/env python3
"""The scattered GDN local-state image must remain an offline-only role."""
import os
import pathlib
import subprocess

assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
layout = pathlib.Path(__file__).resolve().parents[1] / 'PioneerQ38DeltaVectorLayout.h'

def check(defines, accepted, body=''):
    source = ''.join(f'#define {key} {value}\n' for key, value in defines.items())
    source += f'#include "{layout}"\n{body}\n'
    result = subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-Werror', '-fsyntax-only', '-x', 'c++', '-'],
                            input=source, text=True, capture_output=True)
    assert (result.returncode == 0) == accepted, (defines, result.stderr)

candidate = {'PIONEER_Q38_GDN_SCATTERED_STATE_EXPERIMENT':'1'}
check(candidate, True, 'static_assert(PioneerQ38DeltaVectorLayout::StateBytes == 32768);')
for forbidden in ('PIONEER_Q38_WORK_INFERENCE', 'PIONEER_Q38_CPU_VALIDATION',
                  'PIONEER_Q38_FULL_MODEL_CPU_REFERENCE', 'PIONEER_Q38_FULL_OFFLOAD_NATIVE',
                  'PIONEER_Q38_HYBRID_FULL_NATIVE', 'PIONEER_Q38_GDN_RECURRENT_NATIVE', '_KERNEL_MODE'):
    check({**candidate, forbidden:'1'}, False)
check({}, False, '#define PIONEER_Q38_GDN_SCATTERED_STATE_EXPERIMENT 1\n#include "' + str(layout) + '"')
check({**candidate, 'PIONEER_Q38_GDN_INPLACE_STATE_EXPERIMENT':'1'}, False)
check(candidate, False, '#undef PIONEER_Q38_GDN_SCATTERED_STATE_EXPERIMENT\n#include "' + str(layout) + '"')
print('PASS scattered-state role gate: exact 32KiB table role accepted; Work/reference/native/kernel roles and late role addition refused')
