#!/usr/bin/env python3
"""Private backend TEST perimeter only; NEVER link/install/invoke a driver."""
import os
import pathlib
import re
import subprocess

assert not any(key.startswith(('PIONEER_', 'TT_PIONEER_')) for key in os.environ)
here = pathlib.Path(__file__).resolve().parent
kernel = here.parents[2]/'src/add-ons/kernel/drivers/misc'
pattern = r'!defined\(([A-Z_][A-Z0-9_]*)\)\s*\|\|\s*\1\s*!=\s*1\b'
native = set(re.findall(pattern, (kernel/'PioneerQ38TokenNativeRole.h').read_text()))
assert len(native) == 21
abi = ('api_version', 'init_hardware', 'init_driver', 'uninit_driver', 'publish_devices', 'find_device')
accepted = refused = 0


def check(header, definitions, expected, extra=''):
    global accepted, refused
    result = subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-Werror',
                             *['-D'+key+'='+str(value) for key, value in sorted(definitions.items())],
                             '-E', '-x', 'c++', '-'],
                            input='#include "'+str(here/header)+'"\n'+extra, text=True,
                            stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
    assert (result.returncode == 0) == expected, (header, definitions, result.stderr)
    if expected:
        accepted += 1
    else:
        assert 'error:' in result.stderr
        refused += 1


for stem, family, count, inherited, extra_forbidden in (
    ('pioneer_q38_token_native_backend', 'PIONEER_Q38_TOKEN_NATIVE_BACKEND_FIXTURE', 12,
     '',
     ('PIONEER_Q38_TOKEN_NATIVE_BACKEND_INIT_FAULT_FIXTURE', 'PIONEER_Q38_TOKEN_NATIVE_BACKEND_FINISH_FIXTURE', 'PIONEER_Q38_TOKEN_NATIVE_FULL_LAYER_FIXTURE', 'PIONEER_Q38_TOKEN_NATIVE_CANCEL_LAYER_FIXTURE')),
    ('pioneer_q38_token_native_backend_finish', 'PIONEER_Q38_TOKEN_NATIVE_BACKEND_FINISH_FIXTURE', 21,
     '',
     ('PIONEER_Q38_TOKEN_NATIVE_BACKEND_INIT_FAULT_FIXTURE', 'PIONEER_Q38_TOKEN_NATIVE_BACKEND_FIXTURE')),
    ('pioneer_q38_token_native_backend_init_fault', 'PIONEER_Q38_TOKEN_NATIVE_BACKEND_INIT_FAULT_FIXTURE', 7,
     '',
     ('PIONEER_Q38_TOKEN_NATIVE_BACKEND_FIXTURE', 'PIONEER_Q38_TOKEN_NATIVE_BACKEND_FINISH_FIXTURE',
      'PIONEER_Q38_TOKEN_NATIVE_PEER_FIXTURE', 'PIONEER_Q38_TOKEN_NATIVE_TRAFFIC_FIXTURE',
      'PIONEER_Q38_TOKEN_NATIVE_EMBEDDING_FIXTURE', 'PIONEER_Q38_TOKEN_NATIVE_LAYER_FIXTURE',
      'PIONEER_Q38_TOKEN_NATIVE_FULL_LAYER_FIXTURE', 'PIONEER_Q38_TOKEN_NATIVE_CANCEL_LAYER_FIXTURE')),
    ('pioneer_q38_token_native_control', 'PIONEER_Q38_TOKEN_NATIVE_CONTROL_FIXTURE', 8,
     'pioneer_q38_token_native_backend_init_fault_role.h',
     ('PIONEER_Q38_TOKEN_NATIVE_CONTROL_BEGIN_FIXTURE', 'PIONEER_Q38_TOKEN_NATIVE_CONTROL_FINISH_FIXTURE',
      'PIONEER_Q38_TOKEN_NATIVE_CONTROL_OPERATIONS_FIXTURE',
      'PIONEER_Q38_TOKEN_NATIVE_BACKEND_FIXTURE', 'PIONEER_Q38_TOKEN_NATIVE_BACKEND_FINISH_FIXTURE',
      'PIONEER_Q38_TOKEN_NATIVE_PEER_FIXTURE', 'PIONEER_Q38_TOKEN_NATIVE_TRAFFIC_FIXTURE',
      'PIONEER_Q38_TOKEN_NATIVE_EMBEDDING_FIXTURE', 'PIONEER_Q38_TOKEN_NATIVE_LAYER_FIXTURE',
      'PIONEER_Q38_TOKEN_NATIVE_FULL_LAYER_FIXTURE', 'PIONEER_Q38_TOKEN_NATIVE_CANCEL_LAYER_FIXTURE')),
    ('pioneer_q38_token_native_control_begin', 'PIONEER_Q38_TOKEN_NATIVE_CONTROL_BEGIN_FIXTURE', 14,
     'pioneer_q38_token_native_backend_role.h',
     ('PIONEER_Q38_TOKEN_NATIVE_BACKEND_INIT_FAULT_FIXTURE', 'PIONEER_Q38_TOKEN_NATIVE_BACKEND_FINISH_FIXTURE',
      'PIONEER_Q38_TOKEN_NATIVE_CONTROL_FINISH_FIXTURE', 'PIONEER_Q38_TOKEN_NATIVE_CONTROL_OPERATIONS_FIXTURE',
      'PIONEER_Q38_TOKEN_NATIVE_FULL_LAYER_FIXTURE',
      'PIONEER_Q38_TOKEN_NATIVE_CANCEL_LAYER_FIXTURE')),
    ('pioneer_q38_token_native_control_finish', 'PIONEER_Q38_TOKEN_NATIVE_CONTROL_FINISH_FIXTURE', 23,
     'pioneer_q38_token_native_backend_finish_role.h',
     ('PIONEER_Q38_TOKEN_NATIVE_BACKEND_INIT_FAULT_FIXTURE', 'PIONEER_Q38_TOKEN_NATIVE_BACKEND_FIXTURE',
      'PIONEER_Q38_TOKEN_NATIVE_CONTROL_BEGIN_FIXTURE')),
    ('pioneer_q38_token_native_control_operations', 'PIONEER_Q38_TOKEN_NATIVE_CONTROL_OPERATIONS_FIXTURE', 24,
     ('pioneer_q38_token_native_control_finish_role.h', 'pioneer_q38_token_native_backend_finish_role.h'),
     ('PIONEER_Q38_TOKEN_NATIVE_BACKEND_INIT_FAULT_FIXTURE', 'PIONEER_Q38_TOKEN_NATIVE_BACKEND_FIXTURE',
      'PIONEER_Q38_TOKEN_NATIVE_CONTROL_BEGIN_FIXTURE'))):
    role_header = stem+'_role.h'
    fixture = set(re.findall(pattern, (here/role_header).read_text()))
    if inherited:
        for parent in (inherited,) if isinstance(inherited, str) else inherited:
            fixture |= set(re.findall(pattern, (here/parent).read_text()))
    assert len(fixture) == count
    required = native | fixture | {'PIONEER_Q38_TOKEN_PUSHED_PROGRAMS'}
    parents = dict.fromkeys(required, 1)
    parents['PIONEER_DRIVER_BUILD_TAG'] = '0x5154334e4f445256ULL'
    parents.update({name:'pioneer_backend_test_'+name for name in abi})
    for header in (role_header, stem+'_checks.inc'):
        check(header, parents, True)
        for name in sorted(required):
            for value in (None, 0, 2):
                bad = dict(parents)
                if value is None:
                    del bad[name]
                else:
                    bad[name] = value
                check(header, bad, False)
        for name in (*abi, 'PIONEER_DRIVER_BUILD_TAG'):
            bad = dict(parents)
            del bad[name]
            check(header, bad, False)
        for name in (*extra_forbidden, 'PIONEER_Q38_TOKEN_NATIVE_FULL_FLOW_FIXTURE',
                     'PIONEER_Q38_TOKEN_NATIVE_FULL_END_FLOW_FIXTURE', 'PIONEER_Q38_TOKEN_NATIVE_STATE_SNAPSHOT_FIXTURE',
                     'PIONEER_Q38_WORK_INFERENCE', 'TT_PIONEER_Q38_WORK_MODE', 'PIONEER_Q38_CPU_VALIDATION',
                     'PIONEER_Q38_FULL_MODEL_CPU_REFERENCE', 'PIONEER_Q38_TOKEN_PRECISE_NATIVE_CONTEXT',
                     'PIONEER_Q38_TOKEN_PRECISE_PROGRAMS', 'PIONEER_Q38_GLOBAL_CHAIN_PACKET_SEAL_UNROLL'):
            for value in (0, 1):
                check(header, {**parents, name:value}, False)
        # No include guard: a late change must fail too.
        check(header, parents, False, '#undef '+family+'\n#include "'+str(here/role_header)+'"\n')
print(f'PASS private backend TEST perimeter/{accepted} positives/{refused} absent+0+2+ABI+late-change+'
      'full-flow+CPU-reference+Work refusals;NO execution/cleanup/physical/TPS permission')
