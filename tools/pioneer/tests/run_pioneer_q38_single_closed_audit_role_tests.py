#!/usr/bin/env python3
"""Single closed audit BUILD-only isolation;no runtime/device/Work grant."""
import os
import pathlib
import subprocess

assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
here = pathlib.Path(__file__).resolve().parents[1]
role = 'PIONEER_Q38_GLOBAL_CHAIN_SINGLE_CLOSED_AUDIT'
common = {'PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE':1, 'PIONEER_Q38_SIGMOID_PRECISION_IMAGE':1,
          'PIONEER_Q38_LAYER_RETURN_COMPONENT_IMAGE':1, 'PIONEER_Q38_GLOBAL_CHAIN_F32_HIDDEN':1,
          'PIONEER_Q38_GLOBAL_CHAIN_SEAL_UNROLL':1, 'PIONEER_Q38_GLOBAL_CHAIN_BOUNDARY_SEALS':1,
          'PIONEER_Q38_GLOBAL_CHAIN_QUIET_UNROLL':1, 'PIONEER_Q38_GLOBAL_CHAIN_LOCAL_SOURCE_GUARDS':1,
          'PIONEER_Q38_GLOBAL_CHAIN_PRECISE_ACTIVATION':1, role:1}
variants = ({**common, 'PIONEER_Q38_LAYER_COMPONENT_RECURRENT':1,
             'PIONEER_Q38_MIXED_RECURRENT_BLOCK_ROOT_IMAGE':1, 'PIONEER_Q38_GLOBAL_CHAIN_CROSS_TOKEN_COMPONENT':1},
            {**common, 'PIONEER_Q38_LAYER_COMPONENT_DENSE':1,
             'PIONEER_Q38_MIXED_LIVE_BLOCK_ROOT_IMAGE':1, 'PIONEER_Q38_GLOBAL_CHAIN_TOKEN_KV_APPEND':1})
accepted = refused = 0


def check(flags, okay, tail='', header='PioneerQ38GlobalChainComponentRole.h', diagnostic=''):
    global accepted, refused
    include = '#include "'+str(here/header)+'"\n'
    source = ''.join('#define '+name+' '+str(value)+'\n' for name,value in flags.items())+include+tail+include
    result = subprocess.run([os.environ.get('CXX','c++'), '-std=c++17', '-Werror', '-fsyntax-only', '-x','c++','-'],
                            input=source,text=True,stdout=subprocess.DEVNULL,stderr=subprocess.PIPE)
    assert (result.returncode==0)==okay and diagnostic in result.stderr,(flags,tail,result.stderr)
    if okay:
        accepted += 1
    else:
        assert 'error:' in result.stderr
        refused += 1


for flags in variants:
    check(flags, True, 'static_assert(PioneerQ38SingleClosedAuditProfile==1,"policy");\n')
    old = {key:value for key,value in flags.items() if key != role}
    check(old, True, 'static_assert(PioneerQ38SingleClosedAuditProfile==0,"policy");\n')
    check(old, False, '#define '+role+' 1\n', diagnostic='late single-closed-audit policy change')
    check(flags, False, '#undef '+role+'\n', diagnostic='late repeated-closed-audit policy change')
    for name in flags:
        if name != role:
            check({key:value for key,value in flags.items() if key != name}, False)
        for value in (0,2):
            check({**flags,name:value},False)
            check(flags,False,'#undef '+name+'\n#define '+name+' '+str(value)+'\n')
    for name in ('_KERNEL_MODE','__FAST_MATH__','PIONEER_DRIVER_BUILD_TAG','TT_PIONEER_DRIVER_RELOAD',
                 'PIONEER_Q38_GLOBAL_WORKER_IMAGE','PIONEER_Q38_PRECISE_WORKER_IMAGE',
                 'PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE','PIONEER_Q38_GLOBAL_CHAIN_CONV_CAPSULE_IMAGE',
                 'PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE',
                 'PIONEER_Q38_TOKEN_NATIVE_CONTEXT','PIONEER_Q38_TOKEN_PRECISE_NATIVE_CONTEXT',
                 'PIONEER_Q38_TOKEN_PRECISE_PROGRAMS','PIONEER_Q38_FULL_MODEL_CPU_REFERENCE'):
        for value in (0,1):
            check({**flags,name:value},False)
            check(flags,False,'#define '+name+' '+str(value)+'\n')
for header,flags in (
    ('PioneerQ38GlobalWorkerImageRole.h',{'PIONEER_Q38_GLOBAL_WORKER_IMAGE':1}),
    ('PioneerQ38PreciseWorkerImageRole.h',{'PIONEER_Q38_PRECISE_WORKER_IMAGE':1}),
    ('PioneerQ38TokenImageRole.h',{'PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE':1,
                                 'PIONEER_Q38_GLOBAL_CHAIN_FEEDBACK':1,'PIONEER_Q38_TOKEN_CANCEL_BOUNDARY':1}),
    ('PioneerQ38TokenPreciseProgramsRole.h',{'PIONEER_Q38_TOKEN_PRECISE_PROGRAMS':1})):
    check(flags,True,header=header)
    for value in (0,1):
        check({**flags,role:value},False,header=header)
        check(flags,False,'#define '+role+' '+str(value)+'\n',header=header)
print(f'PASS single-closed-audit returning QGW2 BUILD policy/{accepted} positive/{refused} exact-parent+zero+two+late/native/DATA/CPU/worker/Work refusals;NO runtime,physical,combined-stack or speed qualification')
