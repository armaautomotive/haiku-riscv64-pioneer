#!/usr/bin/env python3
"""Experimental ROOT admission BUILD isolation,not execution or Work proof."""
import os
import pathlib
import re
import subprocess

assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
root = pathlib.Path(__file__).resolve().parents[1]
role = 'PIONEER_Q38_GLOBAL_CHAIN_WORKER_SEALED_ADMISSION'
common = dict.fromkeys(('PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE', 'PIONEER_Q38_SIGMOID_PRECISION_IMAGE',
    'PIONEER_Q38_LAYER_RETURN_COMPONENT_IMAGE', 'PIONEER_Q38_GLOBAL_CHAIN_F32_HIDDEN',
    'PIONEER_Q38_GLOBAL_CHAIN_SEAL_UNROLL', 'PIONEER_Q38_GLOBAL_CHAIN_BOUNDARY_SEALS',
    'PIONEER_Q38_GLOBAL_CHAIN_QUIET_UNROLL', 'PIONEER_Q38_GLOBAL_CHAIN_LOCAL_SOURCE_GUARDS',
    'PIONEER_Q38_GLOBAL_CHAIN_PRECISE_ACTIVATION', 'PIONEER_Q38_GLOBAL_CHAIN_SINGLE_CLOSED_AUDIT', role), 1)
variants = ({**common, 'PIONEER_Q38_LAYER_COMPONENT_RECURRENT':1,
             'PIONEER_Q38_MIXED_RECURRENT_BLOCK_ROOT_IMAGE':1, 'PIONEER_Q38_GLOBAL_CHAIN_CROSS_TOKEN_COMPONENT':1},
            {**common, 'PIONEER_Q38_LAYER_COMPONENT_DENSE':1,
             'PIONEER_Q38_MIXED_LIVE_BLOCK_ROOT_IMAGE':1, 'PIONEER_Q38_GLOBAL_CHAIN_TOKEN_KV_APPEND':1})
accepted = refused = 0
def check(flags, okay, tail='', header=None):
    global accepted, refused
    include = '#include "'+str(header or root/'PioneerQ38GlobalChainComponentRole.h')+'"\n'
    source = ''.join('#define '+key+' '+str(value)+'\n' for key,value in flags.items())+include+tail+include
    result = subprocess.run([os.environ.get('CXX','c++'), '-std=c++17', '-Werror', '-fsyntax-only', '-x','c++','-'],
                            input=source,text=True,stdout=subprocess.DEVNULL,stderr=subprocess.PIPE)
    assert (result.returncode==0)==okay,(flags,tail,result.stderr)
    if okay:accepted += 1
    else:
        assert 'error:' in result.stderr
        refused += 1
for flags in variants:
    check(flags,True,'static_assert(PioneerQ38WorkerSealedAdmissionProfile==1,"policy");\n')
    old = {key:value for key,value in flags.items() if key != role}
    check(old,True,'static_assert(PioneerQ38WorkerSealedAdmissionProfile==0,"policy");\n')
    check(old,False,'#define '+role+' 1\n')
    check(flags,False,'#undef '+role+'\n')
    for key in flags:
        check({name:value for name,value in flags.items() if name != key}, key == role)
        for value in (0,2):
            check({**flags,key:value},False)
            check(flags,False,'#undef '+key+'\n#define '+key+' '+str(value)+'\n')
    for key in ('_KERNEL_MODE','__FAST_MATH__','PIONEER_DRIVER_BUILD_TAG','TT_PIONEER_DRIVER_RELOAD',
                'PIONEER_Q38_GLOBAL_WORKER_IMAGE','PIONEER_Q38_PRECISE_WORKER_IMAGE',
                'PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE','PIONEER_Q38_GLOBAL_CHAIN_CONV_CAPSULE_IMAGE',
                'PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE',
                'PIONEER_Q38_TOKEN_NATIVE_CONTEXT','PIONEER_Q38_TOKEN_PRECISE_NATIVE_CONTEXT',
                'PIONEER_Q38_TOKEN_PRECISE_PROGRAMS','PIONEER_Q38_FULL_MODEL_CPU_REFERENCE'):
        for value in (0,1):
            check({**flags,key:value},False)
            check(flags,False,'#define '+key+' '+str(value)+'\n')
native = root.parents[1]/'src/add-ons/kernel/drivers/misc/PioneerQ38TokenNativeRole.h'
parents = set(re.findall(r'!defined\(([A-Z_][A-Z0-9_]*)\)\s*\|\|\s*\1\s*!=\s*1\b', native.read_text()))
assert len(parents)==21
native_flags = dict.fromkeys(parents,1)
native_flags.update(PIONEER_DRIVER_BUILD_TAG='0x51544b4e4f445256ULL')
native_flags.update({name:'pioneer_admission_role_'+name for name in ('api_version','init_hardware','init_driver','uninit_driver','publish_devices','find_device')})
for header,flags in ((root/'PioneerQ38GlobalWorkerImageRole.h',{'PIONEER_Q38_GLOBAL_WORKER_IMAGE':1}),
                     (root/'PioneerQ38PreciseWorkerImageRole.h',{'PIONEER_Q38_PRECISE_WORKER_IMAGE':1}),
                     (root/'PioneerQ38TokenImageRole.h',{'PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE':1,'PIONEER_Q38_GLOBAL_CHAIN_FEEDBACK':1,'PIONEER_Q38_TOKEN_CANCEL_BOUNDARY':1}),
                     (root/'PioneerQ38TokenPreciseProgramsRole.h',{'PIONEER_Q38_TOKEN_PRECISE_PROGRAMS':1}),
                     (native,native_flags)):
    check(flags,True,header=header)
    for value in (0,1):
        check({**flags,role:value},False,header=header)
        check(flags,False,'#define '+role+' '+str(value)+'\n',header=header)
print(f'PASS worker-sealed admission returning QGW2 BUILD policy/{accepted} positive/{refused} missing-parent+zero+two+late/native/DATA/CPU/worker/Work refusals;NO runtime,physical,combined-stack or TPS qualification')
