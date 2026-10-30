#!/usr/bin/env python3
"""Standalone state-DATA TEST role checks; no driver linkage/device IO."""
import os
import pathlib
import re
import subprocess

assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
here = pathlib.Path(__file__).resolve().parent
compiler = os.environ.get('CXX', 'c++')
accepted = refused = 0
def check(header, definitions, okay, syntax=False):
    global accepted, refused
    result = subprocess.run([compiler, '-std=c++17', '-Werror', *[f'-D{k}={v}' for k,v in definitions.items()],
                             '-fsyntax-only' if syntax else '-E', '-x', 'c++', '-'],
                            input=f'#include "{here/header}"\n', stdout=subprocess.DEVNULL, stderr=subprocess.PIPE, text=True)
    assert (result.returncode == 0) == okay, (header, definitions, result.stderr)
    if okay:
        accepted += 1
    else:
        assert 'error:' in result.stderr
        refused += 1
role = (here/'pioneer_q38_token_native_full_end_role.h').read_text()
parents = dict.fromkeys(re.findall(r'!defined\(([A-Z_][A-Z0-9_]*)\)\s*\|\|\s*\1\s*!=\s*1\b', role), 1)
assert len(parents) == 15
parents.update(PIONEER_Q38_NATIVE_STATE_TEST_BRIDGE=1, PIONEER_Q38_TOKEN_NATIVE_STATE_SNAPSHOT_FIXTURE=1)
check('pioneer_q38_native_state_test_bridge.h', {'PIONEER_Q38_NATIVE_STATE_TEST_BRIDGE':1}, True, True)
for header in ('pioneer_q38_native_state_test_bridge.h', 'pioneer_q38_token_native_state_snapshot.inc'):
    base = {**parents, '_KERNEL_MODE':1}
    check(header, base, True)
    for name in parents:
        for value in (None, 0, 2):
            bad = dict(base)
            if value is None:
                del bad[name]
            else:
                bad[name] = value
            check(header, bad, False)
    for name in ('PIONEER_Q38_WORK_INFERENCE', 'TT_PIONEER_Q38_WORK_MODE', 'PIONEER_Q38_FULL_MODEL_CPU_REFERENCE', 'PIONEER_Q38_CPU_VALIDATION',
                 'PIONEER_Q38_TOKEN_NATIVE_CANCEL_LAYER_FIXTURE', 'PIONEER_Q38_TOKEN_NATIVE_HOLD_LAYER_FIXTURE',
                 'PIONEER_Q38_TOKEN_NATIVE_READOUT_EMPTY_FIXTURE', 'PIONEER_Q38_TOKEN_NATIVE_RESTORE_EMPTY_FIXTURE',
                 'PIONEER_Q38_TOKEN_NATIVE_RESTORE_DISPATCH_FIXTURE', 'PIONEER_Q38_TOKEN_NATIVE_RESTORE_PEERS_FIXTURE',
                 'PIONEER_Q38_TOKEN_NATIVE_RESTORE_ROOT_STORE_FIXTURE', 'PIONEER_Q38_TOKEN_NATIVE_RESTORE_SHARED_TLB_FIXTURE',
                 'PIONEER_Q38_TOKEN_NATIVE_HOSTS_FIXTURE', 'PIONEER_ATTENTION_FREESTANDING', 'PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE',
                 'PIONEER_Q38_GLOBAL_WORKER_IMAGE', 'PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE'):
        for value in (0, 1):
            check(header, {**base, name:value}, False)
for name in ('_KERNEL_MODE', 'PIONEER_Q38_TOKEN_NATIVE_CONTEXT', 'PIONEER_Q38_WORK_INFERENCE', 'TT_PIONEER_Q38_WORK_MODE',
             'PIONEER_Q38_FULL_MODEL_CPU_REFERENCE', 'PIONEER_Q38_CPU_VALIDATION', 'PIONEER_ATTENTION_FREESTANDING',
             'PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE', 'PIONEER_Q38_GLOBAL_WORKER_IMAGE', 'PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE'):
    for value in (0, 1):
        check('pioneer_q38_native_state_test_bridge.cpp', {name:value}, False)
check('pioneer_q38_state_snapshot_data.h', {}, True, True)
for name in ('_KERNEL_MODE', 'PIONEER_Q38_TOKEN_NATIVE_CONTEXT', 'PIONEER_Q38_WORK_INFERENCE', 'TT_PIONEER_Q38_WORK_MODE',
             'PIONEER_ATTENTION_FREESTANDING', 'PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE', 'PIONEER_Q38_GLOBAL_WORKER_IMAGE',
             'PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE'):
    for value in (0, 1):
        check('pioneer_q38_state_snapshot_data.h', {name:value}, False)
print(f'PASS state DATA/{accepted} exact TEST roles/{refused} absent+0+2+kernel+CPU-oracle+image+Work refusals/implementation stays HOST-only;NO native owner,execution or cleanup permission')
