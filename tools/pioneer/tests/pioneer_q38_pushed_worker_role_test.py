#!/usr/bin/env python3
"""QGW3 sender BUILD isolation only;never native/image execution authority."""
import os
import pathlib
import re
import subprocess

assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
root=pathlib.Path(__file__).resolve().parents[1]
push='PIONEER_Q38_PUSH_COMPLETION_WORKER_IMAGE'
precise='PIONEER_Q38_PRECISE_WORKER_IMAGE'
header=root/'PioneerQ38PreciseWorkerImageRole.h'
accepted=refused=0

def check(flags,okay,tail='',selected=header):
    global accepted,refused
    include='#include "'+str(selected)+'"\n'
    source=''.join('#define '+key+' '+str(value)+'\n' for key,value in flags.items())+include+tail+include
    result=subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Wall','-Wextra','-Werror','-fsyntax-only','-x','c++','-'],
                          input=source,text=True,capture_output=True)
    assert (result.returncode==0)==okay,(flags,tail,result.stderr)
    if okay:accepted+=1
    else:
        assert 'error:' in result.stderr
        refused+=1

base={precise:1,push:1}
check(base,True,'static_assert(PioneerQ38PreciseWorkerPushProfile==1,"QGW3");\n')
check({precise:1},True,'static_assert(PioneerQ38PreciseWorkerPushProfile==0,"QGW2");\n')
check({precise:1},False,'#define '+push+' 1\n')
check(base,False,'#undef '+push+'\n')
check({push:1},False)
for key in (push,precise):
    for value in (0,2):
        check({**base,key:value},False)
        check(base,False,'#undef '+key+'\n#define '+key+' '+str(value)+'\n')
for key in ('_KERNEL_MODE','__FAST_MATH__','PIONEER_DRIVER_BUILD_TAG','PIONEER_Q38_GLOBAL_WORKER_IMAGE',
            'PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE','PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE',
            'PIONEER_Q38_TOKEN_PRECISE_PROGRAMS','PIONEER_Q38_GLOBAL_CHAIN_PRECISE_ACTIVATION',
            'PIONEER_Q38_GLOBAL_CHAIN_SINGLE_CLOSED_AUDIT','PIONEER_Q38_GLOBAL_CHAIN_WORKER_SEALED_ADMISSION',
            'PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_FULL_MODEL_CPU_REFERENCE','PIONEER_Q38_TOKEN_NATIVE_CONTEXT',
            'PIONEER_Q38_TOKEN_PRECISE_NATIVE_CONTEXT','PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE'):
    for value in (0,1):
        check({**base,key:value},False)
        check(base,False,'#define '+key+' '+str(value)+'\n')
native=root.parents[1]/'src/add-ons/kernel/drivers/misc/PioneerQ38TokenNativeRole.h'
parents=set(re.findall(r'!defined\(([A-Z_][A-Z0-9_]*)\)\s*\|\|\s*\1\s*!=\s*1\b',native.read_text()))
assert len(parents)==21
native_flags=dict.fromkeys(parents,1)
native_flags['PIONEER_DRIVER_BUILD_TAG']='0x51544b4e4f445256ULL'
native_flags.update({name:'pioneer_qgw3_role_'+name for name in ('api_version','init_hardware','init_driver','uninit_driver','publish_devices','find_device')})
for selected,flags in ((root/'PioneerQ38GlobalWorkerImageRole.h',{'PIONEER_Q38_GLOBAL_WORKER_IMAGE':1}),
                       (root/'PioneerQ38TokenPreciseProgramsRole.h',{'PIONEER_Q38_TOKEN_PRECISE_PROGRAMS':1}),
                       (root/'PioneerQ38GlobalChainIsolation.h',{}),(native,native_flags)):
    check(flags,True,selected=selected)
    for value in (0,1):
        check({**flags,push:value},False,selected=selected)
        check(flags,False,'#define '+push+' '+str(value)+'\n',selected=selected)
print(f'PASS QGW3 publisher BUILD isolation/{accepted} positive/{refused} zero+two+late/native+DATA+ROOT+Work+CPU refusals;NO runtime/physical/TPS grant')
