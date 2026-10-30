#!/usr/bin/env python3
"""Rechecked QGW3 receiver BUILD policies;no execution/native authority."""
import os
import pathlib
import subprocess

assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
here=pathlib.Path(__file__).resolve().parent.parent
push='PIONEER_Q38_GLOBAL_CHAIN_PUSHED_COMPLETION'
precise='PIONEER_Q38_GLOBAL_CHAIN_PRECISE_ACTIVATION'
shared=dict.fromkeys(('PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE','PIONEER_Q38_SIGMOID_PRECISION_IMAGE',
 'PIONEER_Q38_LAYER_RETURN_COMPONENT_IMAGE','PIONEER_Q38_GLOBAL_CHAIN_SEAL_UNROLL',
 'PIONEER_Q38_GLOBAL_CHAIN_F32_HIDDEN','PIONEER_Q38_GLOBAL_CHAIN_BOUNDARY_SEALS',
 'PIONEER_Q38_GLOBAL_CHAIN_QUIET_UNROLL','PIONEER_Q38_GLOBAL_CHAIN_LOCAL_SOURCE_GUARDS',
 precise,'PIONEER_Q38_GLOBAL_CHAIN_SINGLE_CLOSED_AUDIT','PIONEER_Q38_GLOBAL_CHAIN_WORKER_SEALED_ADMISSION'),1)
r={**shared,'PIONEER_Q38_LAYER_COMPONENT_RECURRENT':1,'PIONEER_Q38_MIXED_RECURRENT_BLOCK_ROOT_IMAGE':1,
   'PIONEER_Q38_GLOBAL_CHAIN_CROSS_TOKEN_COMPONENT':1}
d={**shared,'PIONEER_Q38_LAYER_COMPONENT_DENSE':1,'PIONEER_Q38_MIXED_LIVE_BLOCK_ROOT_IMAGE':1,
   'PIONEER_Q38_GLOBAL_CHAIN_TOKEN_KV_APPEND':1}
parent=dict.fromkeys(('PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE','PIONEER_Q38_GLOBAL_CHAIN_FEEDBACK',
                     'PIONEER_Q38_TOKEN_CANCEL_BOUNDARY',precise),1)
positive=negative=0
def check(header,flags,okay,tail=''):
    global positive,negative
    include='#include "'+str(here/header)+'"\n'
    source='#include <stdint.h>\n'+''.join('#define '+k+' '+str(v)+'\n' for k,v in flags.items())+include+tail+include
    result=subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Wall','-Wextra','-Werror','-fsyntax-only','-x','c++','-'],input=source,text=True,capture_output=True)
    assert (result.returncode==0)==okay,(header,flags,tail,result.stderr)
    if okay:positive+=1
    else:
        assert 'error:' in result.stderr
        negative+=1

for header,old in (('PioneerQ38GlobalChainComponentRole.h',r),('PioneerQ38GlobalChainComponentRole.h',d),('PioneerQ38TokenImageRole.h',parent)):
    flags={**old,push:1}
    check(header,old,True);check(header,flags,True)
    check(header,old,False,'#define '+push+' 1\n')
    check(header,flags,False,'#undef '+push+'\n')
    for name in flags:
        check(header,{k:v for k,v in flags.items() if k!=name},name==push)
        for value in (0,2):
            check(header,{**flags,name:value},False)
    for name in ('_KERNEL_MODE','__FAST_MATH__','PIONEER_DRIVER_BUILD_TAG','PIONEER_Q38_PUSH_COMPLETION_WORKER_IMAGE',
                 'PIONEER_Q38_PRECISE_WORKER_IMAGE','PIONEER_Q38_GLOBAL_WORKER_IMAGE','PIONEER_Q38_GLOBAL_CHAIN_CONV_CAPSULE_IMAGE',
                 'PIONEER_Q38_TOKEN_PRECISE_PROGRAMS','PIONEER_Q38_TOKEN_NATIVE_CONTEXT','PIONEER_Q38_TOKEN_PRECISE_NATIVE_CONTEXT',
                 'PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_FULL_MODEL_CPU_REFERENCE','PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE'):
        for value in (0,1):
            check(header,{**flags,name:value},False)
            check(header,flags,False,'#define '+name+' '+str(value)+'\n')
for flags in ({},{precise:1},{precise:1,push:1}):
    check('PioneerQ38SelectedGlobalWorker.h',flags,True)
check('PioneerQ38SelectedGlobalWorker.h',{precise:1},False,'#define '+push+' 1\n')
check('PioneerQ38SelectedGlobalWorker.h',{precise:1,push:1},False,'#undef '+push+'\n')
for value in (0,2):check('PioneerQ38SelectedGlobalWorker.h',{precise:1,push:value},False)
packet='PIONEER_Q38_GLOBAL_CHAIN_PACKET_SEAL_UNROLL'
for body in (r,d):
    flags={**body,push:1,packet:1};header='PioneerQ38GlobalChainComponentRole.h'
    check(header,flags,True)
    check(header,{**body,push:1},False,'#define '+packet+' 1\n')
    check(header,flags,False,'#undef '+packet+'\n')
    for name in flags:
        check(header,{k:v for k,v in flags.items() if k!=name},name==packet)
        for value in (0,2):check(header,{**flags,name:value},False)
    for name in ('_KERNEL_MODE','PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE',
                 'PIONEER_Q38_TOKEN_NATIVE_CONTEXT','PIONEER_Q38_TOKEN_PUSHED_PROGRAMS'):
        for value in (0,1):check(header,{**flags,name:value},False)
for value in (0,1,2):
    check('PioneerQ38TokenImageRole.h',{**parent,push:1,packet:value},False)
    check('PioneerQ38GlobalChainIsolation.h',{packet:value},False)
print(f'PASS QGW3 receiver BUILD roles/{positive} positive/{negative} missing+zero+two+late+foreign refusals;NO runtime/native/physical/Work/TPS grant')
