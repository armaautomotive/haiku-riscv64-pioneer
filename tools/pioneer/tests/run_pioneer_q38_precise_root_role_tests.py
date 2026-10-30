#!/usr/bin/env python3
"""Distinct precise ROOT/component roles only; never a native launch grant."""
import os
import pathlib
import subprocess

assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
here = pathlib.Path(__file__).resolve().parents[1]
precise = 'PIONEER_Q38_GLOBAL_CHAIN_PRECISE_ACTIVATION'
parent = {'PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE': 1,
          'PIONEER_Q38_GLOBAL_CHAIN_FEEDBACK': 1, 'PIONEER_Q38_TOKEN_CANCEL_BOUNDARY': 1, precise: 1}
common = {'PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE': 1, 'PIONEER_Q38_SIGMOID_PRECISION_IMAGE': 1,
          'PIONEER_Q38_LAYER_RETURN_COMPONENT_IMAGE': 1, 'PIONEER_Q38_GLOBAL_CHAIN_F32_HIDDEN': 1,
          'PIONEER_Q38_GLOBAL_CHAIN_SEAL_UNROLL': 1, 'PIONEER_Q38_GLOBAL_CHAIN_BOUNDARY_SEALS': 1,
          'PIONEER_Q38_GLOBAL_CHAIN_QUIET_UNROLL': 1, 'PIONEER_Q38_GLOBAL_CHAIN_LOCAL_SOURCE_GUARDS': 1,
          precise: 1}
recurrent = {**common, 'PIONEER_Q38_LAYER_COMPONENT_RECURRENT': 1,
             'PIONEER_Q38_MIXED_RECURRENT_BLOCK_ROOT_IMAGE': 1, 'PIONEER_Q38_GLOBAL_CHAIN_CROSS_TOKEN_COMPONENT': 1}
dense = {**common, 'PIONEER_Q38_LAYER_COMPONENT_DENSE': 1,
         'PIONEER_Q38_MIXED_LIVE_BLOCK_ROOT_IMAGE': 1, 'PIONEER_Q38_GLOBAL_CHAIN_TOKEN_KV_APPEND': 1}
accepted = refused = 0


def check(header, flags, okay, after=''):
    global accepted, refused
    include = '#include "'+str(here/header)+'"\n'
    source = ''.join('#define '+name+' '+str(value)+'\n' for name, value in flags.items())+include+after+include
    result = subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-E', '-x', 'c++', '-'],
                            input=source, text=True, stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
    assert (result.returncode == 0) == okay, (header, flags, after, result.stderr)
    if okay:
        accepted += 1
    else:
        assert 'error:' in result.stderr
        refused += 1


for header, flags in (('PioneerQ38TokenImageRole.h', parent),
                      ('PioneerQ38GlobalChainComponentRole.h', recurrent),
                      ('PioneerQ38GlobalChainComponentRole.h', dense)):
    check(header, flags, True)
    for name in flags:
        for value in (0, 2):
            check(header, {**flags, name: value}, False)
        without = {key: value for key, value in flags.items() if key != name}
        # Without this optional flag the old family is still valid, but it
        # cannot use the new producer/consumer source or QGW2 selection.
        if name != precise:
            check(header, without, False)
        check(header, flags, False, '#undef '+name+'\n#define '+name+' 0\n')
    for name in ('_KERNEL_MODE', '__FAST_MATH__', 'PIONEER_DRIVER_BUILD_TAG',
                 'PIONEER_Q38_GLOBAL_WORKER_IMAGE', 'PIONEER_Q38_PRECISE_WORKER_IMAGE',
                 'PIONEER_Q38_CPU_VALIDATION', 'PIONEER_Q38_WORK_INFERENCE',
                 'TT_PIONEER_Q38_WORK_MODE', 'PIONEER_Q38_TOKEN_NATIVE_CONTEXT',
                 'PIONEER_Q38_FULL_MODEL_CPU_REFERENCE', 'PIONEER_Q38_GLOBAL_CHAIN_CONV_CAPSULE_IMAGE'):
        for value in (0, 1):
            check(header, {**flags, name: value}, False)
            check(header, flags, False, '#define '+name+' '+str(value)+'\n')
for header, flags in (('PioneerQ38GlobalWorkerImageRole.h', {'PIONEER_Q38_GLOBAL_WORKER_IMAGE': 1}),
                      ('PioneerQ38PreciseWorkerImageRole.h', {'PIONEER_Q38_PRECISE_WORKER_IMAGE': 1}),
                      ('PioneerQ38GlobalChainImageRole.h', {'PIONEER_Q38_GLOBAL_CHAIN_ROOT_IMAGE': 1}),
                      ('PioneerQ38GlobalFeedbackComponentRole.h', {'PIONEER_Q38_GLOBAL_CHAIN_FEEDBACK': 1,
                                                                  'PIONEER_Q38_GLOBAL_CHAIN_FEEDBACK_CAPSULE_IMAGE': 1})):
    check(header, flags, True)
    for value in (0, 1):
        check(header, {**flags, precise: value}, False)
        check(header, flags, False, '#define '+precise+' '+str(value)+'\n')
selection = '#include "'+str(here/'PioneerQ38SelectedGlobalWorker.h')+'"\n'
for before, after, okay in (
        ('', '', True), ('#define '+precise+' 1\n', '', True),
        ('', '#define '+precise+' 1\n', False),
        ('#define '+precise+' 1\n', '#undef '+precise+'\n', False),
        ('#define '+precise+' 0\n', '', False), ('#define '+precise+' 2\n', '', False)):
    result = subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-fsyntax-only', '-x', 'c++', '-'],
                            input='#include <stdint.h>\n'+before+selection+after+selection,
                            text=True, stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
    assert (result.returncode == 0) == okay, (before, after, result.stderr)
    if okay:
        accepted += 1
    else:
        assert 'error:' in result.stderr
        refused += 1
print(f'PASS precise ROOT/component roles/positive{accepted}/refusals{refused}/exact flags+late+defined0+old-worker separation;NOT native,physical,Work or launch permission')
