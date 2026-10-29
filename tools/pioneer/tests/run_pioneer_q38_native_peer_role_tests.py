#!/usr/bin/env python3
"""Host peer-engine isolation checks; NEVER link/invoke hardware code."""
import os
import pathlib
import subprocess

assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
here = pathlib.Path(__file__).resolve().parent
role = 'PIONEER_Q38_NATIVE_PEER_TEST_BRIDGE'
fixture = 'PIONEER_Q38_TOKEN_NATIVE_PEER_FIXTURE'
context = 'PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY'
storage = 'PIONEER_Q38_TEST_SHARED_WORKER_STORAGE'
accepted = refused = 0


def check(header, definitions, expected, implementation=False):
    global accepted, refused
    args = [os.environ.get('CXX', 'c++'), '-std=c++17', '-Werror', '-I'+str(here)]
    args += ['-D'+key+'='+str(value) for key, value in definitions.items()]
    if implementation:
        args += ['-E', str(here/header)]
        source = None
    else:
        args += ['-fsyntax-only', '-x', 'c++', '-']
        source = '#include "'+header+'"\n'
    result = subprocess.run(args, input=source, text=True, stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
    assert (result.returncode == 0) == expected, (header, definitions, result.stderr)
    if expected:
        accepted += 1
    else:
        assert 'error:' in result.stderr
        refused += 1


header = 'pioneer_q38_native_peer_test_bridge.h'
check(header, {role:1}, True)
check(header, {role:1, '_KERNEL_MODE':1, fixture:1, context:1}, True)  # opaque interface in emulated fixture ONLY
check('PioneerQ38SharedTestStorage.h', {role:1, storage:1}, True)
for definitions in ({}, {role:0}, {role:2}, {role:1, '_KERNEL_MODE':1},
                    {role:1, '_KERNEL_MODE':1, fixture:1}, {role:1, '_KERNEL_MODE':1, context:1},
                    {role:1, '_KERNEL_MODE':1, fixture:0, context:1}, {role:1, '_KERNEL_MODE':1, fixture:1, context:2}):
    check(header, definitions, False)
for macro in ('PIONEER_Q38_WORK_INFERENCE', 'PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE',
              'PIONEER_Q38_GLOBAL_WORKER_IMAGE', 'PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE'):
    for value in (0, 1):
        check(header, {role:1, macro:value}, False)
for definitions in ({role:1}, {role:1, storage:0}, {role:1, storage:2},
                    {role:1, storage:1, '_KERNEL_MODE':0}, {role:1, storage:1, '_KERNEL_MODE':1}):
    check('PioneerQ38SharedTestStorage.h', definitions, False)
for macro in ('_KERNEL_MODE', 'PIONEER_Q38_TOKEN_NATIVE_CONTEXT', 'PIONEER_Q38_WORK_INFERENCE'):
    for value in (0, 1):
        check('pioneer_q38_native_peer_test_bridge.cpp', {macro:value}, False, True)
for header, family in (
    ('pioneer_q38_token_native_mutable_bytes.inc', 'PIONEER_Q38_TOKEN_NATIVE_FULL_LAYER_FIXTURE'),
    ('pioneer_q38_token_native_full_layer_checks.inc', 'PIONEER_Q38_TOKEN_NATIVE_FULL_LAYER_FIXTURE'),
    ('pioneer_q38_token_native_kv_bytes.inc', 'PIONEER_Q38_TOKEN_NATIVE_FULL_FLOW_FIXTURE'),
    ('pioneer_q38_token_native_full_flow_checks.inc', 'PIONEER_Q38_TOKEN_NATIVE_FULL_FLOW_FIXTURE'),
    ('pioneer_q38_token_native_cancel_layer_checks.inc', 'PIONEER_Q38_TOKEN_NATIVE_CANCEL_LAYER_FIXTURE'),
    ('pioneer_q38_token_native_hold_layer_checks.inc', 'PIONEER_Q38_TOKEN_NATIVE_HOLD_LAYER_FIXTURE'),
    ('pioneer_q38_token_native_readout_empty_checks.inc', 'PIONEER_Q38_TOKEN_NATIVE_READOUT_EMPTY_FIXTURE'),
    ('pioneer_q38_token_native_readout_flow_checks.inc', 'PIONEER_Q38_TOKEN_NATIVE_READOUT_FLOW_FIXTURE'),
    ('pioneer_q38_token_native_restore_empty_checks.inc', 'PIONEER_Q38_TOKEN_NATIVE_RESTORE_EMPTY_FIXTURE'),
    ('pioneer_q38_token_native_dispatch_restore_checks.inc', 'PIONEER_Q38_TOKEN_NATIVE_RESTORE_DISPATCH_FIXTURE'),
    ('pioneer_q38_token_native_peer_restore_checks.inc', 'PIONEER_Q38_TOKEN_NATIVE_RESTORE_PEERS_FIXTURE'),
    ('pioneer_q38_token_native_root_store_restore_checks.inc', 'PIONEER_Q38_TOKEN_NATIVE_RESTORE_ROOT_STORE_FIXTURE'),
    ('pioneer_q38_token_native_shared_tlb_restore_checks.inc', 'PIONEER_Q38_TOKEN_NATIVE_RESTORE_SHARED_TLB_FIXTURE'),
    ('pioneer_q38_token_native_shared_tlb_endpoints.inc', 'PIONEER_Q38_TOKEN_NATIVE_RESTORE_SHARED_TLB_FIXTURE'),
    ('pioneer_q38_token_native_clock_checks.inc', 'PIONEER_Q38_TOKEN_NATIVE_CLOCK_FIXTURE')):
    parents = {family:1, fixture:1, context:1}
    if family in ('PIONEER_Q38_TOKEN_NATIVE_CANCEL_LAYER_FIXTURE','PIONEER_Q38_TOKEN_NATIVE_HOLD_LAYER_FIXTURE'):
        parents['PIONEER_Q38_TOKEN_NATIVE_FULL_LAYER_FIXTURE']=1
    if family=='PIONEER_Q38_TOKEN_NATIVE_HOLD_LAYER_FIXTURE':
        parents['PIONEER_Q38_TOKEN_NATIVE_CANCEL_LAYER_FIXTURE']=1
    if family=='PIONEER_Q38_TOKEN_NATIVE_READOUT_EMPTY_FIXTURE':
        parents.update(PIONEER_Q38_TOKEN_NATIVE_HOLD_LAYER_FIXTURE=1,PIONEER_Q38_TOKEN_NATIVE_CANCEL_LAYER_FIXTURE=1,
                       PIONEER_Q38_TOKEN_NATIVE_FULL_LAYER_FIXTURE=1)
    if family=='PIONEER_Q38_TOKEN_NATIVE_READOUT_FLOW_FIXTURE':
        parents.update(PIONEER_Q38_TOKEN_NATIVE_FULL_FLOW_FIXTURE=1,PIONEER_Q38_TOKEN_NATIVE_FULL_LAYER_FIXTURE=1)
    if family=='PIONEER_Q38_TOKEN_NATIVE_RESTORE_EMPTY_FIXTURE':
        parents.update(PIONEER_Q38_TOKEN_NATIVE_READOUT_EMPTY_FIXTURE=1,PIONEER_Q38_TOKEN_NATIVE_HOLD_LAYER_FIXTURE=1,
                       PIONEER_Q38_TOKEN_NATIVE_CANCEL_LAYER_FIXTURE=1,PIONEER_Q38_TOKEN_NATIVE_FULL_LAYER_FIXTURE=1)
    if family=='PIONEER_Q38_TOKEN_NATIVE_RESTORE_DISPATCH_FIXTURE':
        parents.update(PIONEER_Q38_TOKEN_NATIVE_RESTORE_EMPTY_FIXTURE=1,PIONEER_Q38_TOKEN_NATIVE_READOUT_EMPTY_FIXTURE=1,
                       PIONEER_Q38_TOKEN_NATIVE_HOLD_LAYER_FIXTURE=1,PIONEER_Q38_TOKEN_NATIVE_CANCEL_LAYER_FIXTURE=1,
                       PIONEER_Q38_TOKEN_NATIVE_FULL_LAYER_FIXTURE=1)
    if family in ('PIONEER_Q38_TOKEN_NATIVE_RESTORE_PEERS_FIXTURE','PIONEER_Q38_TOKEN_NATIVE_RESTORE_ROOT_STORE_FIXTURE','PIONEER_Q38_TOKEN_NATIVE_RESTORE_SHARED_TLB_FIXTURE','PIONEER_Q38_TOKEN_NATIVE_CLOCK_FIXTURE'):
        parents.update(PIONEER_Q38_TOKEN_NATIVE_RESTORE_DISPATCH_FIXTURE=1,PIONEER_Q38_TOKEN_NATIVE_RESTORE_EMPTY_FIXTURE=1,
                       PIONEER_Q38_TOKEN_NATIVE_READOUT_EMPTY_FIXTURE=1,PIONEER_Q38_TOKEN_NATIVE_HOLD_LAYER_FIXTURE=1,
                       PIONEER_Q38_TOKEN_NATIVE_CANCEL_LAYER_FIXTURE=1,PIONEER_Q38_TOKEN_NATIVE_FULL_LAYER_FIXTURE=1)
    if family in ('PIONEER_Q38_TOKEN_NATIVE_RESTORE_ROOT_STORE_FIXTURE','PIONEER_Q38_TOKEN_NATIVE_RESTORE_SHARED_TLB_FIXTURE','PIONEER_Q38_TOKEN_NATIVE_CLOCK_FIXTURE'):
        parents['PIONEER_Q38_TOKEN_NATIVE_RESTORE_PEERS_FIXTURE']=1
    if family in ('PIONEER_Q38_TOKEN_NATIVE_RESTORE_SHARED_TLB_FIXTURE','PIONEER_Q38_TOKEN_NATIVE_CLOCK_FIXTURE'):
        parents['PIONEER_Q38_TOKEN_NATIVE_RESTORE_ROOT_STORE_FIXTURE']=1
    if family=='PIONEER_Q38_TOKEN_NATIVE_CLOCK_FIXTURE':
        parents['PIONEER_Q38_TOKEN_NATIVE_RESTORE_SHARED_TLB_FIXTURE']=1
    cases = [(parents, True)]
    for macro in parents:
        absent = dict(parents)
        del absent[macro]
        cases.append((absent, False))
        for value in (0, 2):
            cases.append(({**parents, macro:value}, False))
    for value in (0, 1):
        cases.append(({**parents, 'PIONEER_Q38_WORK_INFERENCE':value}, False))
        if family in ('PIONEER_Q38_TOKEN_NATIVE_RESTORE_ROOT_STORE_FIXTURE','PIONEER_Q38_TOKEN_NATIVE_RESTORE_SHARED_TLB_FIXTURE','PIONEER_Q38_TOKEN_NATIVE_CLOCK_FIXTURE'):
            cases.append(({**parents, 'PIONEER_Q38_TOKEN_NATIVE_FULL_FLOW_FIXTURE':value}, False))
    for definitions, expected in cases:
        result = subprocess.run([os.environ.get('CXX', 'c++'), '-E', '-x', 'c++',
                                 *['-D'+key+'='+str(value) for key, value in definitions.items()], str(here/header)],
                                stdout=subprocess.DEVNULL, stderr=subprocess.PIPE, text=True)
        assert (result.returncode == 0) == expected, (header, definitions, result.stderr)
        if expected:
            accepted += 1  # preprocess role only;NOT a native owner or usable component
        else:
            assert 'Explicit native' in result.stderr
            refused += 1
trace = 'PIONEER_Q38_NATIVE_TRACE_TEST_BRIDGE'
flow = 'PIONEER_Q38_TOKEN_NATIVE_FULL_FLOW_FIXTURE'
header = 'pioneer_q38_native_trace_test_bridge.h'
check(header, {trace:1}, True)
check(header, {trace:1, '_KERNEL_MODE':1, flow:1, context:1}, True)
for definitions in ({}, {trace:0}, {trace:2}, {trace:1, '_KERNEL_MODE':1},
                    {trace:1, '_KERNEL_MODE':1, flow:1}, {trace:1, '_KERNEL_MODE':1, context:1},
                    {trace:1, '_KERNEL_MODE':1, flow:0, context:1}, {trace:1, '_KERNEL_MODE':1, flow:2, context:1},
                    {trace:1, '_KERNEL_MODE':1, flow:1, context:0}, {trace:1, '_KERNEL_MODE':1, flow:1, context:2}):
    check(header, definitions, False)
for macro in ('PIONEER_Q38_WORK_INFERENCE', 'PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE',
              'PIONEER_Q38_GLOBAL_WORKER_IMAGE', 'PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE', 'PIONEER_Q38_FULL_MODEL_CPU_REFERENCE'):
    for value in (0, 1):
        check(header, {trace:1, macro:value}, False)
for macro in ('_KERNEL_MODE', 'PIONEER_Q38_TOKEN_NATIVE_CONTEXT', 'PIONEER_Q38_WORK_INFERENCE'):
    for value in (0, 1):
        check('pioneer_q38_native_trace_test_bridge.cpp', {macro:value}, False, True)
print(f'PASS peer-engine byte interface/storage/{accepted} explicit TEST roles/{refused} absent+zero+two+kernel+device+Work refusals/implementation rejects ALL native roles;NO native permission,processor-history setter or product supplier')
