#!/usr/bin/env python3
"""Exact tag106 native/USER/observer DATA separation;no live driver grant."""
import os
from pathlib import Path
import re
import subprocess
import sys

here=Path(__file__).resolve().parents[1]
driver=here.parents[1]/'src/add-ons/kernel/drivers/misc'
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
paired=sys.argv[1:]==['--pair-issue']
assert paired or not sys.argv[1:]
flags={name:1 for name in re.findall(r'^#define ([A-Z][A-Z0-9_]+) 1$',(driver/'pioneer_q38_bf16_full_driver.cpp').read_text(),re.M)}
flags.update({'_KERNEL_MODE':1,'TT_PIONEER_DRIVER_RELOAD':1,'PIONEER_DRIVER_BUILD_TAG':'0x2026110500010105ULL'})
flags.update({name:1 for name in re.findall(r'^#define ([A-Z][A-Z0-9_]+) 1$',(driver/'pioneer_q38_bf16_profile_full_driver.cpp').read_text(),re.M)})
flags['PIONEER_DRIVER_BUILD_TAG']='0x2026110500010106ULL'
if paired:
 flags.update({'PIONEER_Q38_BF16_PAIR_ISSUE_FULL':1,'PIONEER_Q38_TOKEN_BF16_PAIR_ISSUE_PROGRAMS':1,
  'PIONEER_DRIVER_BUILD_TAG':'0x2026110600010109ULL'})
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
  'PIONEER_Q38_FULL_OFFLOAD_BATCHED','PIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4',
  'PIONEER_Q38_FULL_OFFLOAD_RESIDENT','PIONEER_Q38_TOKEN_VALIDATION_NATIVE','PIONEER_Q38_TOKEN_HIDDEN_VALIDATION_NATIVE',
  'PIONEER_Q38_BF16_PROFILE_IMAGE','PIONEER_Q38_TOKEN_PHASE_PROFILE_PROGRAMS',
  '__FAST_MATH__','api_version','init_hardware','init_driver','uninit_driver','publish_devices','find_device'):
  for value in (0,1):
   check(header,{**flags,name:value},False)
   check(header,flags,False,'#define '+name+' '+str(value)+'\n')
selected=here/'PioneerQ38TokenSelectedPrograms.h'
data={'PIONEER_Q38_TOKEN_BF16_PROGRAMS':1,'PIONEER_Q38_TOKEN_BF16_PROFILE_PROGRAMS':1}
if paired:data['PIONEER_Q38_TOKEN_BF16_PAIR_ISSUE_PROGRAMS']=1
check(selected,data,True,body='static_assert(PioneerQ38TokenPrograms::Family==0x34505451 && PioneerQ38TokenPrograms::Version=='+str(3 if paired else 2)+' && PioneerQ38TokenPrograms::TokenPhaseProfile==1 && PioneerQ38TokenL1::SavedBytes==61778152);\n')
check(selected,flags,True)
check(selected,{'PIONEER_Q38_TOKEN_BF16_PROGRAMS':0},False)
check(selected,{},False,'#define PIONEER_Q38_TOKEN_BF16_PROGRAMS 1\n')
check(selected,{'PIONEER_Q38_TOKEN_BF16_PROGRAMS':1},False,'#undef PIONEER_Q38_TOKEN_BF16_PROGRAMS\n')
boundary=(here/'PioneerQ38TokenBf16DataBoundary.h').read_text()
for name in sorted(set(re.findall(r'defined\((PIONEER_Q38_TOKEN_[A-Z0-9_]+_PROGRAMS)\)',boundary))-{'PIONEER_Q38_TOKEN_BF16_PROGRAMS'}):
 for value in (0,1):
  check(selected,{**data,name:value},False)
  check(selected,data,False,'#define '+name+' '+str(value)+'\n')
check(selected,data,False,'#include "'+str(here/'PioneerQ38TokenBf16Programs.h')+'"\n')
check(selected,{'PIONEER_Q38_TOKEN_BF16_PROGRAMS':1},False,'#define PIONEER_Q38_TOKEN_BF16_PROFILE_PROGRAMS 1\n')
check(selected,data,False,'#undef PIONEER_Q38_TOKEN_BF16_PROFILE_PROGRAMS\n')
check(selected,{'PIONEER_Q38_TOKEN_BF16_PROFILE_PROGRAMS':1},False)
client={'PIONEER_Q38_BF16_FULL_CLIENT':1,'PIONEER_Q38_FULL_OFFLOAD_CLIENT':1,
 'PIONEER_Q38_BF16_FULL_PROFILE':1,'PIONEER_Q38_FULL_OFFLOAD_METRICS':1,'PIONEER_Q38_FULL_OFFLOAD_PHASE_PROFILE':1}
if paired:client['PIONEER_Q38_BF16_PAIR_ISSUE_FULL']=1
header=here/'PioneerQ38Bf16FullClientRole.h';check(header,client,True)
for name in client:
 if paired and name=='PIONEER_Q38_BF16_PAIR_ISSUE_FULL':
  # With no fixed kernel tag,a FIRST include without the optional paired
  # selector is legitimately the old106 USER subject,never109 authority.
  check(header,{k:v for k,v in client.items() if k!=name},True,body=
   '#define B_DEVICE_OP_CODES_END 1000\n#include "'+str(driver/'PioneerQ38Bf16FullProtocol.h')+'"\n'
   'static_assert(PioneerQ38FullOffload::BuildTag==UINT64_C(0x2026110500010106));\n')
 else:check(header,{k:v for k,v in client.items() if k!=name},False)
 for value in (0,2):check(header,{**client,name:value},False)
 check(header,client,False,'#undef '+name+'\n#define '+name+' 0\n')
for name in ('_KERNEL_MODE','PIONEER_Q38_BF16_FULL_NATIVE','PIONEER_Q38_TOKEN_BF16_PROFILE_PROGRAMS',
 'PIONEER_Q38_BF16_PROFILE_IMAGE','PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_FULL_MODEL_CPU_REFERENCE','PIONEER_Q38_WORK_INFERENCE'):
 for value in (0,1):check(header,{**client,name:value},False)
check(here/'PioneerQ38SelectedPhaseProfileData.h',{},False,'#define PIONEER_Q38_BF16_FULL_PROFILE 1\n')
check(here/'PioneerQ38SelectedPhaseProfileData.h',{'PIONEER_Q38_BF16_FULL_PROFILE':1},False,'#undef PIONEER_Q38_BF16_FULL_PROFILE\n')
if paired:
 for tag in ('0x2026110500010105ULL','0x2026110500010106ULL','0x2026110600010108ULL'):
  check(driver/'PioneerQ38Bf16FullNativeRole.h',{**flags,'PIONEER_DRIVER_BUILD_TAG':tag},False)
 for macro in ('PIONEER_Q38_CACHED_FPU_PAIR_ISSUE_IMAGE','PIONEER_Q38_Q4_PAIR_ISSUE_IMAGE','PIONEER_NATIVE_COMPACT_Q4_PAIR_ISSUE_TIMING'):
  for value in (0,1):
   check(driver/'PioneerQ38Bf16FullNativeRole.h',{**flags,macro:value},False)
   check(header,{**client,macro:value},False)
 check(selected,data,False,'#include "'+str(here/'PioneerQ38TokenBf16ProfilePrograms.h')+'"\n')
 check(selected,data,False,'#undef PIONEER_Q38_TOKEN_BF16_PAIR_ISSUE_PROGRAMS\n')
 check(header,client,False,'#undef PIONEER_Q38_BF16_PAIR_ISSUE_FULL\n')
 check(header,{k:v for k,v in client.items() if k!='PIONEER_Q38_BF16_PAIR_ISSUE_FULL'},False,
  '#define PIONEER_Q38_BF16_PAIR_ISSUE_FULL 1\n')
print(f'PASS exact BF16 timing native/USER/DATA roles positive{positive}/refusals{refused}/missing+zero+late+old+CPU+Work separation;NOT hardware/lifecycle/model-quality/TPS permission')
