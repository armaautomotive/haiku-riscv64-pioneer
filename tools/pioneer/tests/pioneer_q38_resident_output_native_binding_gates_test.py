#!/usr/bin/env python3
"""New native state role cannot change after inclusion; NO runtime grant."""
import os
import pathlib
import subprocess

kernel=pathlib.Path(__file__).resolve().parents[3]/'src/add-ons/kernel/drivers/misc'
header='#include "'+str(kernel/'PioneerQ38ResidentOutputNativeState.h')+'"\n'
flags=dict(PIONEER_Q38_RESIDENT_OUTPUT_OWNER='1',PIONEER_Q38_PROJECTION_EXECUTION_OWNER='1',
           PIONEER_Q38_PROJECTION_RESULT_STASH='1',PIONEER_Q38_LEARNED_RMS_NATIVE='1',
           PIONEER_Q38_EMBEDDING_NATIVE='1',PIONEER_Q38_DECODER_RUNTIME='1',PIONEER_Q38_GDN_MUTABLE_STATE='')
clean={k:v for k,v in os.environ.items() if not k.startswith('PIONEER_')}
count=0
def check(source,roles,expected):
    global count
    result=subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-E','-x','c++',
                           *['-D'+k+'='+v for k,v in roles.items()],'-'],input=source,text=True,
                          capture_output=True,env=clean,timeout=10)
    assert (result.returncode==0)==expected,(roles,result.stderr)
    count+=1
check(header,flags,True)
for key in flags:
    roles=dict(flags);del roles[key];check(header,roles,False)
    check(header+'#undef '+key+'\n'+header,flags,False)
    if key!='PIONEER_Q38_GDN_MUTABLE_STATE':
        for value in ('0','2','-1',''):
            check(header,{**flags,key:value},False)
            check(header+'#undef '+key+'\n#define '+key+' '+value+'\n'+header,flags,False)
for alias in ('PIONEER_Q38_RESIDENT_DECODER_OWNER','PIONEER_Q38_RESIDENT_DECODER_NATIVE',
              'PIONEER_Q38_GDN_RECURRENT_NATIVE','PIONEER_Q38_GDN_INPUT_OWNER','PIONEER_Q38_CPU_VALIDATION',
              'PIONEER_Q38_WORK_INFERENCE','PIONEER_Q38_GDN_OUTPUT_VALIDATION_NATIVE',
              'PIONEER_Q38_RESIDENT_OUTPUT_ROOT_IMAGE'):
    for value in ('0','1'):
        check(header,{**flags,alias:value},False)
        check(header+'#define '+alias+' '+value+'\n'+header,flags,False)
print('PASS new native binding state role gates'+str(count)+'/exact parents+runtime/defined0+late changes+old-owner+CPU+DEBUG+Work+image exclusion;private metadata ONLY,NO native execution or IO grant')
