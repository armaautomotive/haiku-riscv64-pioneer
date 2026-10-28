#!/usr/bin/env python3
"""Separate prefix validation readback/late owner and native role refusals."""
import os
import pathlib
import re
import subprocess
here=pathlib.Path(__file__).resolve().parent;kernel=here.parents[2]/'src/add-ons/kernel/drivers/misc'
role=kernel/'PioneerQ38RecurrentPrefixReadbackRole.h';selection='PIONEER_Q38_RECURRENT_PREFIX_VALIDATION_READBACK'
forbidden=set(re.findall(r'(?<!!)defined\(([A-Z_][A-Z0-9_]*)\)',role.read_text()))-{selection}
assert {'PIONEER_Q38_WORK_INFERENCE','PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_RECURRENT_BLOCK_NATIVE_CONTEXT',
 'PIONEER_Q38_RECURRENT_PREFIX_SCHEDULER_IMAGE','__FAST_MATH__'}<=forbidden
clean={k:v for k,v in os.environ.items() if not k.startswith(('PIONEER_','TT_PIONEER_'))}
include='#include "'+str(role)+'"\n';count=0
def check(defines,okay,text=None,syntax=False,extra=()):
 global count
 source=''.join('#define '+k+' '+v+'\n' for k,v in sorted(defines.items()))+(include if text is None else text)
 command=[os.environ.get('CXX','c++'),'-std=c++17',*(['-fsyntax-only'] if syntax else ['-E','-P']),*extra,'-x','c++','-']
 result=subprocess.run(command,input=source,text=True,stdout=subprocess.DEVNULL,stderr=subprocess.PIPE,env=clean,timeout=30)
 assert (result.returncode==0)==okay,(defines,text,result.stderr);count+=1
base={selection:'1'};check(base,True);check({},False)
for value in ('0','2','-1',''):
 check({selection:value},False);check(base,False,include+'#undef '+selection+'\n#define '+selection+' '+value+'\n'+include)
for key in sorted(forbidden):
 for value in ('0','1'):
  check({**base,key:value},False);check(base,False,include+'#define '+key+' '+value+'\n'+include)
native=(kernel/'PioneerQ38RecurrentPrefixNativeRole.h').read_text()
parents=set(re.findall(r'!defined\(([A-Z][A-Z0-9_]*)\)\s*\|\|\s*\1\s*!=\s*1\b',native));assert len(parents)==21
names=('api_version','init_hardware','init_driver','uninit_driver','publish_devices','find_device')
valid={**base,**{k:'1' for k in parents},'PIONEER_DRIVER_BUILD_TAG':'0x5152504e4f445256ULL',**{n:'checked_'+n for n in names}}
check(valid,True);check({**valid,'PIONEER_Q38_RECURRENT_PREFIX_FINAL_RELEASE':'1'},True)
check({**base,'PIONEER_Q38_RECURRENT_PREFIX_NATIVE_CONTEXT':'1'},False)
for key in ('PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE','PIONEER_Q38_CPU_VALIDATION'):
 for value in ('0','1'):check({**valid,key:value},False)
for tag in ('0','0x5152424e4f445256ULL','0x2026102700010072ULL','0x2026102100010012ULL'):
 check({**valid,'PIONEER_DRIVER_BUILD_TAG':tag},False)
flags=re.findall(r'-D[A-Z][A-Z0-9_]*(?:=[0-9]+)?',(here/'run_pioneer_q38_ffn_persistent_image_tests.sh').read_text())
fixture='#define PIONEER_Q38_RECURRENT_PREFIX_OWNER_ACTUAL_FIXTURE_ONLY\n#include "'+str(here/'pioneer_q38_recurrent_prefix_owner_actual_test.cpp')+'"\n'
owner='#include "'+str(kernel/'PioneerQ38RecurrentPrefixOwner.h')+'"\n'
check(base,True,fixture+owner,True,flags)
check(base,False,fixture+'#undef '+selection+'\n'+owner,True,flags)
check({},False,fixture+'#define '+selection+' 1\n'+owner,True,flags)
print('PASS DISTINCT prefix readback gates'+str(count)+'/exact component+21parent renamed-native perimeter/defined0+late owner selection+CPU/Work/old block+image aliases refused;NO public or physical authority')
