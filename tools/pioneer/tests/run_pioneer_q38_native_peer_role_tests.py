#!/usr/bin/env python3
"""Host peer-engine isolation checks; NEVER link/invoke hardware code."""
import os
import pathlib
import re
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
        if header.endswith('.inc'):
            args += ['-x', 'c++']
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
check('pioneer_q38_native_peer_test_bridge.cpp', {'PIONEER_Q38_NATIVE_PEER_RECURRENT_TEST':1}, True, True)
for value in (0,2):
    check('pioneer_q38_native_peer_test_bridge.cpp', {'PIONEER_Q38_NATIVE_PEER_RECURRENT_TEST':value}, False, True)
for macro in ('_KERNEL_MODE','PIONEER_Q38_TOKEN_NATIVE_CONTEXT','PIONEER_Q38_WORK_INFERENCE'):
    check('pioneer_q38_native_peer_test_bridge.cpp', {'PIONEER_Q38_NATIVE_PEER_RECURRENT_TEST':1,macro:1}, False, True)
matrix_host={'PIONEER_Q38_NATIVE_PEER_RECURRENT_MATRIX_TEST':1,'PIONEER_Q38_NATIVE_PEER_RECURRENT_TEST':1,
             'PIONEER_Q38_TEST_BF16_CONNECTED_PREPARE':1,'PIONEER_Q38_TOKEN_BF16_PROGRAMS':1}
check('pioneer_q38_native_peer_test_bridge.cpp',matrix_host,True,True)
for parent in matrix_host:
    invalid=dict(matrix_host);del invalid[parent]
    check('pioneer_q38_native_peer_test_bridge.cpp',invalid,parent=='PIONEER_Q38_NATIVE_PEER_RECURRENT_MATRIX_TEST',True)
for macro in ('_KERNEL_MODE','PIONEER_Q38_TOKEN_NATIVE_CONTEXT','PIONEER_Q38_WORK_INFERENCE'):
    check('pioneer_q38_native_peer_test_bridge.cpp',{**matrix_host,macro:1},False,True)
bfp_header='pioneer_q38_native_bfp_peer_bridge.h'
check(bfp_header,{role:1,**matrix_host,'TT_PIONEER_Q38_WORK_MODE':1},True)
for parent in ('PIONEER_Q38_NATIVE_PEER_RECURRENT_MATRIX_TEST','PIONEER_Q38_NATIVE_PEER_RECURRENT_TEST','PIONEER_Q38_TEST_BF16_CONNECTED_PREPARE'):
    invalid={role:1,**matrix_host,'TT_PIONEER_Q38_WORK_MODE':1};invalid[parent]=0
    check(bfp_header,invalid,False)
for macro in ('_KERNEL_MODE','PIONEER_Q38_WORK_INFERENCE','PIONEER_Q38_FULL_OFFLOAD_NATIVE'):
    check(bfp_header,{role:1,**matrix_host,'TT_PIONEER_Q38_WORK_MODE':1,macro:1},False)
recurrent_bytes={'PIONEER_Q38_TOKEN_NATIVE_RECURRENT_PEER_FIXTURE':1,
                 'PIONEER_Q38_RECURRENT_KERNEL_COMPILE_ONLY':1,fixture:1}
check('pioneer_q38_token_native_peer_bytes.inc',recurrent_bytes,True,True)
for parent in recurrent_bytes:
    for value in (None,0,2):
        invalid=dict(recurrent_bytes)
        if value is None:del invalid[parent]
        else:invalid[parent]=value
        # Omitting the selector is the ordinary legacy byte fixture.
        check('pioneer_q38_token_native_peer_bytes.inc',invalid,parent=='PIONEER_Q38_TOKEN_NATIVE_RECURRENT_PEER_FIXTURE' and value is None,True)
for macro in ('PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE'):
    check('pioneer_q38_token_native_peer_bytes.inc',{**recurrent_bytes,macro:1},False,True)
producer={'PIONEER_Q38_TOKEN_NATIVE_RECURRENT_PRODUCER_FIXTURE':1,
          'PIONEER_Q38_TOKEN_NATIVE_RECURRENT_PEER_FIXTURE':1,
          'PIONEER_Q38_TOKEN_NATIVE_LAYER_FIXTURE':1,'PIONEER_Q38_SYNTHETIC_EMBEDDING_TEST':1}
check('pioneer_q38_token_native_execution_checks.inc',producer,True,True)
for parent in producer:
    invalid=dict(producer);del invalid[parent]
    check('pioneer_q38_token_native_execution_checks.inc',invalid,parent=='PIONEER_Q38_TOKEN_NATIVE_RECURRENT_PRODUCER_FIXTURE',True)
for macro in ('PIONEER_Q38_TOKEN_NATIVE_FIRST_MATRIX_FIXTURE','PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE'):
    check('pioneer_q38_token_native_execution_checks.inc',{**producer,macro:1},False,True)
cohort={**producer,'PIONEER_Q38_TOKEN_NATIVE_RECURRENT_PRODUCERS_FIXTURE':1}
check('pioneer_q38_token_native_execution_checks.inc',cohort,True,True)
for value in (0,2):
    check('pioneer_q38_token_native_execution_checks.inc',{**cohort,'PIONEER_Q38_TOKEN_NATIVE_RECURRENT_PRODUCERS_FIXTURE':value},False,True)
check('pioneer_q38_token_native_execution_checks.inc',{'PIONEER_Q38_TOKEN_NATIVE_RECURRENT_PRODUCERS_FIXTURE':1},False,True)
matrix={**cohort,'PIONEER_Q38_TOKEN_NATIVE_SYNTHETIC_RECURRENT_MATRIX_FIXTURE':1}
check('pioneer_q38_token_native_execution_checks.inc',matrix,True,True)
for value in (0,2):
    check('pioneer_q38_token_native_execution_checks.inc',{**matrix,'PIONEER_Q38_TOKEN_NATIVE_SYNTHETIC_RECURRENT_MATRIX_FIXTURE':value},False,True)
check('pioneer_q38_token_native_execution_checks.inc',{'PIONEER_Q38_TOKEN_NATIVE_SYNTHETIC_RECURRENT_MATRIX_FIXTURE':1},False,True)
matrix_execution={**matrix,'PIONEER_Q38_TOKEN_NATIVE_RECURRENT_MATRIX_FIXTURE':1}
check('pioneer_q38_token_native_execution_checks.inc',matrix_execution,True,True)
for value in (0,2):
    check('pioneer_q38_token_native_execution_checks.inc',{**matrix_execution,'PIONEER_Q38_TOKEN_NATIVE_RECURRENT_MATRIX_FIXTURE':value},False,True)
check('pioneer_q38_token_native_execution_checks.inc',{'PIONEER_Q38_TOKEN_NATIVE_RECURRENT_MATRIX_FIXTURE':1},False,True)
matrix_cohort={**matrix_execution,'PIONEER_Q38_TOKEN_NATIVE_RECURRENT_MATRIX_COHORT_FIXTURE':1}
check('pioneer_q38_token_native_execution_checks.inc',matrix_cohort,True,True)
for value in (0,2):
    check('pioneer_q38_token_native_execution_checks.inc',{**matrix_cohort,'PIONEER_Q38_TOKEN_NATIVE_RECURRENT_MATRIX_COHORT_FIXTURE':value},False,True)
check('pioneer_q38_token_native_execution_checks.inc',{'PIONEER_Q38_TOKEN_NATIVE_RECURRENT_MATRIX_COHORT_FIXTURE':1},False,True)
matrix_reuse={**matrix_cohort,'PIONEER_Q38_TOKEN_NATIVE_RECURRENT_MATRIX_REUSE_FIXTURE':1}
check('pioneer_q38_token_native_execution_checks.inc',matrix_reuse,True,True)
for value in (0,2):
    check('pioneer_q38_token_native_execution_checks.inc',{**matrix_reuse,'PIONEER_Q38_TOKEN_NATIVE_RECURRENT_MATRIX_REUSE_FIXTURE':value},False,True)
check('pioneer_q38_token_native_execution_checks.inc',{'PIONEER_Q38_TOKEN_NATIVE_RECURRENT_MATRIX_REUSE_FIXTURE':1},False,True)
small_to_gate={**matrix_reuse,'PIONEER_Q38_TOKEN_NATIVE_SMALL_TO_GATE_FIXTURE':1}
check('pioneer_q38_token_native_execution_checks.inc',small_to_gate,True,True)
for value in (0,2):
    check('pioneer_q38_token_native_execution_checks.inc',{**small_to_gate,'PIONEER_Q38_TOKEN_NATIVE_SMALL_TO_GATE_FIXTURE':value},False,True)
check('pioneer_q38_token_native_execution_checks.inc',{'PIONEER_Q38_TOKEN_NATIVE_SMALL_TO_GATE_FIXTURE':1},False,True)
gate_cohort={**small_to_gate,'PIONEER_Q38_TOKEN_NATIVE_GATE_COHORT_FIXTURE':1}
check('pioneer_q38_token_native_execution_checks.inc',gate_cohort,True,True)
for value in (0,2):
    check('pioneer_q38_token_native_execution_checks.inc',{**gate_cohort,'PIONEER_Q38_TOKEN_NATIVE_GATE_COHORT_FIXTURE':value},False,True)
check('pioneer_q38_token_native_execution_checks.inc',{'PIONEER_Q38_TOKEN_NATIVE_GATE_COHORT_FIXTURE':1},False,True)
qkv_cohort={**gate_cohort,'PIONEER_Q38_TOKEN_NATIVE_QKV_COHORT_FIXTURE':1}
check('pioneer_q38_token_native_execution_checks.inc',qkv_cohort,True,True)
for value in (0,2):
    check('pioneer_q38_token_native_execution_checks.inc',{**qkv_cohort,'PIONEER_Q38_TOKEN_NATIVE_QKV_COHORT_FIXTURE':value},False,True)
check('pioneer_q38_token_native_execution_checks.inc',{'PIONEER_Q38_TOKEN_NATIVE_QKV_COHORT_FIXTURE':1},False,True)
qkv_fault={**qkv_cohort,'PIONEER_Q38_TOKEN_NATIVE_QKV_CONV_RECEIPT_FAULT_FIXTURE':1,'PIONEER_Q38_TOKEN_NATIVE_QKV_CONV_FAULT_KIND':1}
check('pioneer_q38_token_native_execution_checks.inc',qkv_fault,True,True)
for value in (0,2):
    check('pioneer_q38_token_native_execution_checks.inc',{**qkv_fault,'PIONEER_Q38_TOKEN_NATIVE_QKV_CONV_RECEIPT_FAULT_FIXTURE':value},False,True)
check('pioneer_q38_token_native_execution_checks.inc',{'PIONEER_Q38_TOKEN_NATIVE_QKV_CONV_RECEIPT_FAULT_FIXTURE':1},False,True)
for value in (0,4):
    check('pioneer_q38_token_native_execution_checks.inc',{**qkv_fault,'PIONEER_Q38_TOKEN_NATIVE_QKV_CONV_FAULT_KIND':value},False,True)
check('pioneer_q38_token_native_execution_checks.inc',{'PIONEER_Q38_TOKEN_NATIVE_QKV_CONV_FAULT_KIND':1},False,True)
model_observer={**matrix_reuse,'PIONEER_Q38_MODEL_RECURRENT_OBSERVER_TEST':1}
check('pioneer_q38_token_native_execution_checks.inc',model_observer,True,True)
for value in (0,2):
    check('pioneer_q38_token_native_execution_checks.inc',{**model_observer,'PIONEER_Q38_MODEL_RECURRENT_OBSERVER_TEST':value},False,True)
check('pioneer_q38_token_native_execution_checks.inc',{'PIONEER_Q38_MODEL_RECURRENT_OBSERVER_TEST':1},False,True)
stack_header='PioneerQ38InstructionStackObserver.h'
stack_role='PIONEER_Q38_INSTRUCTION_STACK_OBSERVER_TEST'
check(stack_header,{stack_role:1},True)
for value in (None,0,2):
    check(stack_header,{} if value is None else {stack_role:value},False)
stack_kernel={stack_role:1,'_KERNEL_MODE':1,'PIONEER_Q38_TOKEN_NATIVE_CONTEXT':1,
              context:1,'PIONEER_Q38_TOKEN_NATIVE_LAYER_FIXTURE':1}
check(stack_header,stack_kernel,True)
for parent in stack_kernel:
    for value in (None,0,2):
        invalid=dict(stack_kernel)
        if value is None:del invalid[parent]
        else:invalid[parent]=value
        check(stack_header,invalid,False)
for macro in ('PIONEER_Q38_WORK_INFERENCE','PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE',
              'PIONEER_Q38_GLOBAL_WORKER_IMAGE','PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE'):
    for value in (0,1):check(stack_header,{stack_role:1,macro:value},False)
for value in (0,1):
    check(stack_header,{stack_role:1,'TT_PIONEER_Q38_WORK_MODE':value},False)
    check(stack_header,{stack_role:1,'TT_PIONEER_Q38_WORK_MODE':value,role:1,storage:1},True)
    check(stack_header,{stack_role:1,'TT_PIONEER_Q38_WORK_MODE':value,role:1},False)
    check(stack_header,{**stack_kernel,'TT_PIONEER_Q38_WORK_MODE':value,role:1,storage:1},False)
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
    ('pioneer_q38_token_native_clock_checks.inc', 'PIONEER_Q38_TOKEN_NATIVE_CLOCK_FIXTURE'),
    ('pioneer_q38_token_native_hardware_seal_checks.inc', 'PIONEER_Q38_TOKEN_NATIVE_HARDWARE_SEAL_FIXTURE'),
    ('pioneer_q38_token_native_views_checks.inc', 'PIONEER_Q38_TOKEN_NATIVE_VIEWS_FIXTURE'),
    ('pioneer_q38_token_native_pci_checks.inc', 'PIONEER_Q38_TOKEN_NATIVE_PCI_FIXTURE'),
    ('pioneer_q38_token_native_hosts_checks.inc', 'PIONEER_Q38_TOKEN_NATIVE_HOSTS_FIXTURE')):
    parents = {family:1, fixture:1, context:1}
    # New descendants inherit the complete hardware-seal TEST perimeter.
    host_family = family=='PIONEER_Q38_TOKEN_NATIVE_HOSTS_FIXTURE'
    if host_family:
        parents['PIONEER_Q38_TOKEN_NATIVE_PCI_FIXTURE']=1
    pci_family = host_family or family=='PIONEER_Q38_TOKEN_NATIVE_PCI_FIXTURE'
    view_family = pci_family or family=='PIONEER_Q38_TOKEN_NATIVE_VIEWS_FIXTURE'
    if pci_family:
        parents['PIONEER_Q38_TOKEN_NATIVE_VIEWS_FIXTURE']=1
    if view_family:
        parents.update(PIONEER_Q38_TOKEN_NATIVE_HARDWARE_SEAL_FIXTURE=1,PIONEER_Q38_TOKEN_NATIVE_CLOCK_FIXTURE=1,
                       PIONEER_Q38_TOKEN_NATIVE_RESTORE_SHARED_TLB_FIXTURE=1,PIONEER_Q38_TOKEN_NATIVE_RESTORE_ROOT_STORE_FIXTURE=1,
                       PIONEER_Q38_TOKEN_NATIVE_RESTORE_PEERS_FIXTURE=1,PIONEER_Q38_TOKEN_NATIVE_RESTORE_DISPATCH_FIXTURE=1,
                       PIONEER_Q38_TOKEN_NATIVE_RESTORE_EMPTY_FIXTURE=1,PIONEER_Q38_TOKEN_NATIVE_READOUT_EMPTY_FIXTURE=1,
                       PIONEER_Q38_TOKEN_NATIVE_HOLD_LAYER_FIXTURE=1,PIONEER_Q38_TOKEN_NATIVE_CANCEL_LAYER_FIXTURE=1,
                       PIONEER_Q38_TOKEN_NATIVE_FULL_LAYER_FIXTURE=1)
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
    if family in ('PIONEER_Q38_TOKEN_NATIVE_RESTORE_PEERS_FIXTURE','PIONEER_Q38_TOKEN_NATIVE_RESTORE_ROOT_STORE_FIXTURE','PIONEER_Q38_TOKEN_NATIVE_RESTORE_SHARED_TLB_FIXTURE','PIONEER_Q38_TOKEN_NATIVE_CLOCK_FIXTURE','PIONEER_Q38_TOKEN_NATIVE_HARDWARE_SEAL_FIXTURE'):
        parents.update(PIONEER_Q38_TOKEN_NATIVE_RESTORE_DISPATCH_FIXTURE=1,PIONEER_Q38_TOKEN_NATIVE_RESTORE_EMPTY_FIXTURE=1,
                       PIONEER_Q38_TOKEN_NATIVE_READOUT_EMPTY_FIXTURE=1,PIONEER_Q38_TOKEN_NATIVE_HOLD_LAYER_FIXTURE=1,
                       PIONEER_Q38_TOKEN_NATIVE_CANCEL_LAYER_FIXTURE=1,PIONEER_Q38_TOKEN_NATIVE_FULL_LAYER_FIXTURE=1)
    if family in ('PIONEER_Q38_TOKEN_NATIVE_RESTORE_ROOT_STORE_FIXTURE','PIONEER_Q38_TOKEN_NATIVE_RESTORE_SHARED_TLB_FIXTURE','PIONEER_Q38_TOKEN_NATIVE_CLOCK_FIXTURE','PIONEER_Q38_TOKEN_NATIVE_HARDWARE_SEAL_FIXTURE'):
        parents['PIONEER_Q38_TOKEN_NATIVE_RESTORE_PEERS_FIXTURE']=1
    if family in ('PIONEER_Q38_TOKEN_NATIVE_RESTORE_SHARED_TLB_FIXTURE','PIONEER_Q38_TOKEN_NATIVE_CLOCK_FIXTURE','PIONEER_Q38_TOKEN_NATIVE_HARDWARE_SEAL_FIXTURE'):
        parents['PIONEER_Q38_TOKEN_NATIVE_RESTORE_ROOT_STORE_FIXTURE']=1
    if family in ('PIONEER_Q38_TOKEN_NATIVE_CLOCK_FIXTURE','PIONEER_Q38_TOKEN_NATIVE_HARDWARE_SEAL_FIXTURE'):
        parents['PIONEER_Q38_TOKEN_NATIVE_RESTORE_SHARED_TLB_FIXTURE']=1
    if family=='PIONEER_Q38_TOKEN_NATIVE_HARDWARE_SEAL_FIXTURE':
        parents['PIONEER_Q38_TOKEN_NATIVE_CLOCK_FIXTURE']=1
    cases = [(parents, True)]
    for macro in parents:
        absent = dict(parents)
        del absent[macro]
        cases.append((absent, False))
        for value in (0, 2):
            cases.append(({**parents, macro:value}, False))
    for value in (0, 1):
        cases.append(({**parents, 'PIONEER_Q38_WORK_INFERENCE':value}, False))
        if view_family or family in ('PIONEER_Q38_TOKEN_NATIVE_RESTORE_ROOT_STORE_FIXTURE','PIONEER_Q38_TOKEN_NATIVE_RESTORE_SHARED_TLB_FIXTURE','PIONEER_Q38_TOKEN_NATIVE_CLOCK_FIXTURE','PIONEER_Q38_TOKEN_NATIVE_HARDWARE_SEAL_FIXTURE'):
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
# This separate complete-generation cleanup TEST must not acquire the
# cancellation/empty-readout ancestors rejected by their original helpers.
end_role = (here/'pioneer_q38_token_native_full_end_role.h').read_text()
end_parents = dict.fromkeys(re.findall(r'!defined\(([A-Z_][A-Z0-9_]*)\)\s*\|\|\s*\1\s*!=\s*1\b', end_role), 1)
assert len(end_parents) == 15
end_cases = [(end_parents, True)]
for macro in end_parents:
    absent = dict(end_parents)
    del absent[macro]
    end_cases.append((absent, False))
    for value in (0, 2):
        end_cases.append(({**end_parents, macro:value}, False))
end_forbidden = ('PIONEER_Q38_WORK_INFERENCE', 'PIONEER_Q38_FULL_MODEL_CPU_REFERENCE', 'PIONEER_Q38_CPU_VALIDATION',
    'TT_PIONEER_Q38_WORK_MODE', 'PIONEER_Q38_TOKEN_NATIVE_CANCEL_LAYER_FIXTURE', 'PIONEER_Q38_TOKEN_NATIVE_HOLD_LAYER_FIXTURE',
    'PIONEER_Q38_TOKEN_NATIVE_READOUT_EMPTY_FIXTURE', 'PIONEER_Q38_TOKEN_NATIVE_RESTORE_EMPTY_FIXTURE',
    'PIONEER_Q38_TOKEN_NATIVE_RESTORE_DISPATCH_FIXTURE', 'PIONEER_Q38_TOKEN_NATIVE_RESTORE_PEERS_FIXTURE',
    'PIONEER_Q38_TOKEN_NATIVE_RESTORE_ROOT_STORE_FIXTURE', 'PIONEER_Q38_TOKEN_NATIVE_RESTORE_SHARED_TLB_FIXTURE',
    'PIONEER_Q38_TOKEN_NATIVE_HOSTS_FIXTURE')
for macro in end_forbidden:
    for value in (0, 1):
        end_cases.append(({**end_parents, macro:value}, False))
for header in ('pioneer_q38_token_native_full_end_role.h', 'pioneer_q38_token_native_full_end_endpoints.inc',
               'pioneer_q38_token_native_full_end_checks.inc', 'pioneer_q38_token_clock_raw_role.h'):
    for definitions, expected in end_cases:
        result = subprocess.run([os.environ.get('CXX', 'c++'), '-E', '-x', 'c++',
            *['-D'+key+'='+str(value) for key, value in definitions.items()], str(here/header)],
            stdout=subprocess.DEVNULL, stderr=subprocess.PIPE, text=True)
        assert (result.returncode == 0) == expected, (header, definitions, result.stderr)
        if expected:
            accepted += 1
        else:
            assert 'Explicit native' in result.stderr
            refused += 1
clock_only = {'PIONEER_Q38_TOKEN_NATIVE_CONTEXT':1, 'PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY':1}
check('pioneer_q38_token_clock_raw_role.h', clock_only, True)
for macro in clock_only:
    for value in (0, 2):
        check('pioneer_q38_token_clock_raw_role.h', {**clock_only, macro:value}, False)
for macro in ('PIONEER_Q38_WORK_INFERENCE', 'PIONEER_Q38_TOKEN_NATIVE_FULL_FLOW_FIXTURE', 'PIONEER_Q38_TOKEN_NATIVE_FULL_END_FLOW_FIXTURE'):
    for value in (0, 1):
        check('pioneer_q38_token_clock_raw_role.h', {**clock_only, macro:value}, False)
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
state = 'PIONEER_Q38_NATIVE_STATE_TEST_BRIDGE'
state_fixture = 'PIONEER_Q38_TOKEN_NATIVE_STATE_SNAPSHOT_FIXTURE'
state_parents = {**end_parents, state:1, state_fixture:1}
check('pioneer_q38_native_state_test_bridge.h', {state:1}, True)
for header in ('pioneer_q38_native_state_test_bridge.h', 'pioneer_q38_token_native_state_snapshot.inc'):
    # The opaque bridge additionally accepts an exact emulated-kernel role;
    # the collector itself always needs its full17-parent TEST perimeter.
    cases = [({**state_parents, '_KERNEL_MODE':1}, True)]
    for parent in state_parents:
        for value in (None, 0, 2):
            definitions = {**state_parents, '_KERNEL_MODE':1}
            if value is None:
                del definitions[parent]
            else:
                definitions[parent] = value
            cases.append((definitions, False))
    for macro in (*end_forbidden, 'PIONEER_ATTENTION_FREESTANDING', 'PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE',
                  'PIONEER_Q38_GLOBAL_WORKER_IMAGE', 'PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE'):
        for value in (0, 1):
            cases.append(({**state_parents, '_KERNEL_MODE':1, macro:value}, False))
    for definitions, expected in cases:
        result = subprocess.run([os.environ.get('CXX', 'c++'), '-E', '-x', 'c++',
            *['-D'+key+'='+str(value) for key, value in definitions.items()], str(here/header)],
            stdout=subprocess.DEVNULL, stderr=subprocess.PIPE, text=True)
        assert (result.returncode == 0) == expected, (header, definitions, result.stderr)
        if expected:
            accepted += 1
        else:
            assert 'error:' in result.stderr
            refused += 1
for macro in ('_KERNEL_MODE', 'PIONEER_Q38_TOKEN_NATIVE_CONTEXT', 'PIONEER_Q38_WORK_INFERENCE',
              'PIONEER_Q38_FULL_MODEL_CPU_REFERENCE', 'PIONEER_Q38_CPU_VALIDATION'):
    for value in (0, 1):
        check('pioneer_q38_native_state_test_bridge.cpp', {macro:value}, False, True)
check('pioneer_q38_state_snapshot_data.h', {}, True)
for macro in ('_KERNEL_MODE', 'PIONEER_Q38_TOKEN_NATIVE_CONTEXT', 'PIONEER_Q38_WORK_INFERENCE', 'TT_PIONEER_Q38_WORK_MODE',
              'PIONEER_ATTENTION_FREESTANDING', 'PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE', 'PIONEER_Q38_GLOBAL_WORKER_IMAGE',
              'PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE'):
    for value in (0, 1):
        check('pioneer_q38_state_snapshot_data.h', {macro:value}, False)
print(f'PASS peer-engine byte interface/storage/{accepted} explicit TEST roles/{refused} absent+zero+two+kernel+device+Work refusals/implementation rejects ALL native roles;NO native permission,processor-history setter or product supplier')
