#!/usr/bin/env python3
"""Exact BF16 native selection and DATA separation;no live driver grant."""
import os
from pathlib import Path
import re
import subprocess

here=Path(__file__).resolve().parents[1]
driver=here.parents[1]/'src/add-ons/kernel/drivers/misc'
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
flags={name:1 for name in re.findall(r'^#define ([A-Z][A-Z0-9_]+) 1$',(driver/'pioneer_q38_bf16_full_driver.cpp').read_text(),re.M)}
flags.update({'_KERNEL_MODE':1,'TT_PIONEER_DRIVER_RELOAD':1,'PIONEER_DRIVER_BUILD_TAG':'0x2026110500010105ULL'})
for name in ('BANKS','PANEL','COHORT','DISPATCH','DISPATCH_VECTOR','STREAM'):flags['TT_PIONEER_EXPERIMENTAL_Q38_'+name]=1
positive=refused=0

def check(header,defines,okay,after='',body=''):
 global positive,refused
 include='#include "'+str(header)+'"\n'
 source='#include <stdint.h>\n'+''.join('#define '+name+' '+str(value)+'\n' for name,value in defines.items())
 result=subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Wall','-Wextra','-Werror','-fsyntax-only','-x','c++','-'],
  input=source+include+after+include+body,text=True,stdout=subprocess.DEVNULL,stderr=subprocess.PIPE)
 assert (result.returncode==0)==okay,(header,defines,after,body,result.stderr)
 if okay:positive+=1
 else:assert 'error:' in result.stderr;refused+=1

for header in (driver/'PioneerQ38Bf16FullNativeRole.h',driver/'PioneerQ38TokenNativeRole.h'):
 check(header,flags,True,body='static_assert(PioneerQ38TokenNativeProfile==4 && PioneerQ38TokenNativePhysicalSelection==4);\n')
 for name in flags:
  check(header,{k:v for k,v in flags.items() if k!=name},False)
  for value in (0,2):check(header,{**flags,name:value},False)
  check(header,flags,False,'#undef '+name+'\n#define '+name+' 0\n')
 for name in ('PIONEER_Q38_TOKEN_PUSHED_NATIVE_CONTEXT','PIONEER_Q38_TOKEN_PUSHED_PROGRAMS',
  'PIONEER_Q38_TOKEN_PRECISE_NATIVE_CONTEXT','PIONEER_Q38_TOKEN_PRECISE_PROGRAMS',
  'PIONEER_Q38_FULL_MODEL_CPU_REFERENCE','PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_WORK_INFERENCE',
  'TT_PIONEER_Q38_WORK_MODE','PIONEER_Q38_GLOBAL_CHAIN_BF16_FPU','PIONEER_Q38_BF16_WORKER_IMAGE',
  'PIONEER_Q38_FULL_OFFLOAD_BATCHED','PIONEER_Q38_FULL_OFFLOAD_METRICS','PIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4',
  'PIONEER_Q38_FULL_OFFLOAD_RESIDENT','PIONEER_Q38_TOKEN_VALIDATION_NATIVE','PIONEER_Q38_TOKEN_HIDDEN_VALIDATION_NATIVE',
  '__FAST_MATH__','api_version','init_hardware','init_driver','uninit_driver','publish_devices','find_device'):
  for value in (0,1):
   check(header,{**flags,name:value},False)
   check(header,flags,False,'#define '+name+' '+str(value)+'\n')
selected=here/'PioneerQ38TokenSelectedPrograms.h'
check(selected,{'PIONEER_Q38_TOKEN_BF16_PROGRAMS':1},True,body='static_assert(PioneerQ38TokenPrograms::Family==0x34505451 && PioneerQ38TokenL1::SavedBytes==61778152);\n')
check(selected,flags,True)
check(selected,{'PIONEER_Q38_TOKEN_BF16_PROGRAMS':0},False)
check(selected,{},False,'#define PIONEER_Q38_TOKEN_BF16_PROGRAMS 1\n')
check(selected,{'PIONEER_Q38_TOKEN_BF16_PROGRAMS':1},False,'#undef PIONEER_Q38_TOKEN_BF16_PROGRAMS\n')
boundary=(here/'PioneerQ38TokenBf16DataBoundary.h').read_text()
for name in sorted(set(re.findall(r'defined\((PIONEER_Q38_TOKEN_[A-Z0-9_]+_PROGRAMS)\)',boundary))-{'PIONEER_Q38_TOKEN_BF16_PROGRAMS'}):
 for value in (0,1):
  check(selected,{'PIONEER_Q38_TOKEN_BF16_PROGRAMS':1,name:value},False)
  check(selected,{'PIONEER_Q38_TOKEN_BF16_PROGRAMS':1},False,'#define '+name+' '+str(value)+'\n')
check(selected,{'PIONEER_Q38_TOKEN_BF16_PROGRAMS':1},False,'#include "'+str(here/'PioneerQ38TokenPushedPrograms.h')+'"\n')
print(f'PASS exact BF16 native/immutable DATA roles positive{positive}/refusals{refused}/missing+zero+late+old+CPU+Work separation;NOT hardware/lifecycle/model-quality/TPS permission')
