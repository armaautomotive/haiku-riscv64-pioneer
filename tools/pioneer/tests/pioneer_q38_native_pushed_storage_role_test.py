#!/usr/bin/env python3
"""G3 HOST byte bridge isolation;never native/device/Work execution."""
import os
import pathlib
import subprocess

assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
here=pathlib.Path(__file__).resolve().parent
accepted=refused=0
fabric={'PIONEER_Q38_NATIVE_PUSHED_FABRIC_TEST':1,'PIONEER_Q38_TOKEN_PUSHED_PROGRAMS':1}
def check(definitions,expected,implementation=False):
    global accepted,refused
    args=[os.environ.get('CXX','c++'),'-std=c++17','-Werror','-I'+str(here),
          *['-D'+key+'='+str(value) for key,value in definitions.items()]]
    if implementation:
        args+=['-E',str(here/'pioneer_q38_native_peer_test_bridge.cpp')]
        text=None
    else:
        args+=['-fsyntax-only','-x','c++','-']
        text='#include "PioneerQ38NativePushedTestFabric.h"\n'
    result=subprocess.run(args,input=text,text=True,stdout=subprocess.DEVNULL,stderr=subprocess.PIPE,timeout=60)
    assert (result.returncode==0)==expected,(definitions,implementation,result.stderr)
    if expected:accepted+=1
    else:
        assert 'error:' in result.stderr
        refused+=1
check(fabric,True)
fixture={'_KERNEL_MODE':1,'PIONEER_Q38_TOKEN_NATIVE_PEER_FIXTURE':1,'PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY':1}
check({**fabric,**fixture},True) # simulated caller types only,not real kernel/native ownership
for name in fabric:
    check({k:v for k,v in fabric.items() if k!=name},False)
    for value in (0,2):check({**fabric,name:value},False)
for name in ('PIONEER_Q38_TOKEN_PRECISE_PROGRAMS','PIONEER_Q38_TOKEN_PRECISE_NATIVE_CONTEXT',
             'PIONEER_Q38_GLOBAL_CHAIN_PUSHED_COMPLETION','PIONEER_Q38_GLOBAL_CHAIN_PRECISE_ACTIVATION',
             'PIONEER_Q38_WORK_INFERENCE','PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE',
             'PIONEER_Q38_GLOBAL_WORKER_IMAGE','PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE'):
    for value in (0,1):check({**fabric,name:value},False)
check({**fabric,'_KERNEL_MODE':1},False)
for name in ('PIONEER_Q38_TOKEN_NATIVE_PEER_FIXTURE','PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY'):
    check({**fabric,**{k:v for k,v in fixture.items() if k!=name}},False)
    for value in (0,2):check({**fabric,**fixture,name:value},False)
check({},True,True) # unchanged G1 implementation
check({'PIONEER_Q38_TOKEN_PRECISE_PROGRAMS':1},True,True) # unchanged G2 DATA path
check({'PIONEER_Q38_TOKEN_PUSHED_PROGRAMS':1},True,True)
for name in ('_KERNEL_MODE','PIONEER_Q38_TOKEN_NATIVE_CONTEXT','PIONEER_Q38_WORK_INFERENCE',
             'PIONEER_Q38_TOKEN_PUSHED_NATIVE_CONTEXT','PIONEER_Q38_TOKEN_PRECISE_NATIVE_CONTEXT',
             'PIONEER_Q38_GLOBAL_CHAIN_PUSHED_COMPLETION','PIONEER_Q38_GLOBAL_CHAIN_PRECISE_ACTIVATION'):
    for value in (0,1):check({'PIONEER_Q38_TOKEN_PUSHED_PROGRAMS':1,name:value},False,True)
for name in ('PIONEER_Q38_TOKEN_PRECISE_PROGRAMS',):
    for value in (0,1):check({'PIONEER_Q38_TOKEN_PUSHED_PROGRAMS':1,name:value},False,True)
for value in (0,2):check({'PIONEER_Q38_TOKEN_PUSHED_PROGRAMS':value},False,True)
print(f'PASS G3 HOST byte-fabric/bridge roles/{accepted} positive/{refused} missing+zero+two+foreign+BUILD+native+Work refusals;NO runtime/native ownership/physical/Work/TPS grant')
