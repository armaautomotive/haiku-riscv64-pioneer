#!/usr/bin/env python3
"""Arithmetic role isolation, including attempts after the include guard."""
import os
import pathlib
import subprocess

assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
here = pathlib.Path(__file__).resolve().parent
header = here.parent/'PioneerQ38PreciseActivationChunk.h'
parents = {'PIONEER_Q38_PRECISE_ACTIVATION_CHUNK_IMAGE': 1, 'PIONEER_Q38_SIGMOID_PRECISION_IMAGE': 1}
forbidden = ('_KERNEL_MODE', '__FAST_MATH__', 'PIONEER_Q38_CPU_VALIDATION', 'PIONEER_Q38_WORK_INFERENCE',
             'TT_PIONEER_Q38_WORK_MODE', 'PIONEER_Q38_MIXED_LIVE_NATIVE_CONTEXT',
             'PIONEER_Q38_MIXED_LIVE_VALIDATION_NATIVE', 'PIONEER_Q38_MIXED_LIVE_VALIDATION_CLIENT',
             'PIONEER_DRIVER_BUILD_TAG', 'PIONEER_Q38_TOKEN_NATIVE_CONTEXT', 'PIONEER_Q38_GLOBAL_WORKER_IMAGE',
             'PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE', 'PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE',
             'PIONEER_Q38_FULL_MODEL_CPU_REFERENCE', 'PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY',
             'PIONEER_Q38_SILU_PRECISION_IMAGE', 'PIONEER_Q38_SILU_LEGACY_CONTROL_IMAGE')
accepted = refused = 0


def check(definitions, okay, late=''):
    global accepted, refused
    source = ''.join('#define '+k+' '+str(v)+'\n' for k, v in definitions.items())
    source += '#include "'+str(header)+'"\n'
    source += late
    result = subprocess.run([os.environ.get('CXX', 'c++'), '-E', '-x', 'c++', '-'],
                            input=source, stdout=subprocess.DEVNULL, stderr=subprocess.PIPE, text=True)
    assert (result.returncode == 0) == okay, (definitions, late, result.stderr)
    if okay:
        accepted += 1
    else:
        assert 'error:' in result.stderr
        refused += 1


check(parents, True)
check(parents, True, '#include "'+str(header)+'"\n')
for name in parents:
    for value in (None, 0, 2):
        definitions = dict(parents)
        if value is None:
            del definitions[name]
        else:
            definitions[name] = value
        check(definitions, False)
        late = '#undef '+name+'\n'
        if value is not None:
            late += '#define '+name+' '+str(value)+'\n'
        check(parents, False, late+'#include "'+str(header)+'"\n')
for name in forbidden:
    for value in (0, 1):
        check({**parents, name: value}, False)
        check(parents, False, '#define '+name+' '+str(value)+'\n#include "'+str(header)+'"\n')
print(f'PASS QAC1 isolation/positive{accepted}/refusals{refused}/missing+zero+two+late native,CPU-reference,Work,fast-math,old-SiLU roles;NO native grant')
