#!/usr/bin/env python3
"""Compile-time isolation checks for the private GDN L1 alias experiment."""
import os
import pathlib
import subprocess

assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
here = pathlib.Path(__file__).resolve().parent
layout = here.parent / 'PioneerQ38DeltaVectorLayout.h'

def check(defines, accepted, body=''):
    source = ''.join(f'#define {key} {value}\n' for key, value in defines.items())
    source += f'#include "{layout}"\n{body}\n'
    result = subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-Werror', '-fsyntax-only', '-x', 'c++', '-'],
                            input=source, text=True, capture_output=True)
    assert (result.returncode == 0) == accepted, (defines, result.stderr)

image = {'PIONEER_Q38_RESIDENT_OPERATOR_IMAGE':'1', 'PIONEER_Q38_RESIDENT_GDN_OPERATOR_IMAGE':'1',
         'PIONEER_Q38_FFN_PERSISTENT':'1', 'PIONEER_Q38_PROJECTION_PERSISTENT':'1'}
check(image, True, 'static_assert(PioneerQ38DeltaVectorLayout::Prior != PioneerQ38DeltaVectorLayout::Next);')
candidate = {**image, 'PIONEER_Q38_GDN_INPLACE_STATE_EXPERIMENT':'1'}
check(candidate, True, 'static_assert(PioneerQ38DeltaVectorLayout::Prior == PioneerQ38DeltaVectorLayout::Next);')
for forbidden in ('PIONEER_Q38_WORK_INFERENCE', 'PIONEER_Q38_CPU_VALIDATION',
                  'PIONEER_Q38_FULL_MODEL_CPU_REFERENCE', 'PIONEER_Q38_FULL_OFFLOAD_NATIVE',
                  'PIONEER_Q38_HYBRID_FULL_NATIVE', '_KERNEL_MODE'):
    check({**candidate, forbidden:'1'}, False)
check({'PIONEER_Q38_GDN_INPLACE_STATE_EXPERIMENT':'1'}, False)
check(candidate, False, '#undef PIONEER_Q38_GDN_INPLACE_STATE_EXPERIMENT\n#include "' + str(layout) + '"')
print('PASS GDN in-place experiment compile gates: default image unchanged; candidate aliases Prior/Next only under exact returning-image role; Work/CPU-validation/native/kernel contexts and late role removal refused')
