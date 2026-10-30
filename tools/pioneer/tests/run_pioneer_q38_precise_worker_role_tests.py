#!/usr/bin/env python3
"""QGW2 image compile-role isolation; no native execution grant."""
import os
import pathlib
import subprocess

assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
header = pathlib.Path(__file__).resolve().parents[1]/'PioneerQ38PreciseWorkerImageRole.h'
role = 'PIONEER_Q38_PRECISE_WORKER_IMAGE'
forbidden = ('_KERNEL_MODE', '__FAST_MATH__', 'PIONEER_DRIVER_BUILD_TAG', 'PIONEER_Q38_GLOBAL_WORKER_IMAGE',
             'PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE', 'PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE',
             'PIONEER_Q38_MIXED_WORKER_IMAGE', 'PIONEER_Q38_MIXED_CAPSULE_IMAGE', 'PIONEER_Q38_CPU_VALIDATION',
             'PIONEER_Q38_WORK_INFERENCE', 'TT_PIONEER_Q38_WORK_MODE', 'PIONEER_Q38_TOKEN_NATIVE_CONTEXT',
             'PIONEER_Q38_FULL_MODEL_CPU_REFERENCE')
accepted = refused = 0


def check(prefix, okay):
    global accepted, refused
    result = subprocess.run([os.environ.get('CXX', 'c++'), '-E', '-x', 'c++', '-'],
                            input=prefix+'#include "'+str(header)+'"\n', stdout=subprocess.DEVNULL,
                            stderr=subprocess.PIPE, text=True)
    assert (result.returncode == 0) == okay, (prefix, result.stderr)
    if okay:
        accepted += 1
    else:
        assert 'error:' in result.stderr
        refused += 1


valid = '#define '+role+' 1\n'
check(valid, True)
check(valid+'#include "'+str(header)+'"\n', True)
for value in (None, 0, 2):
    prefix = '' if value is None else '#define '+role+' '+str(value)+'\n'
    check(prefix, False)
    check(valid+'#include "'+str(header)+'"\n#undef '+role+'\n'+prefix, False)
for name in forbidden:
    for value in (0, 1):
        contamination = '#define '+name+' '+str(value)+'\n'
        check(valid+contamination, False)
        check(valid+'#include "'+str(header)+'"\n'+contamination, False)
print(f'PASS QGW2 image roles/positive{accepted}/refusals{refused}/absent+zero+two+late old-worker,ROOT,kernel,CPU-reference,Work contamination;NO native grant')
