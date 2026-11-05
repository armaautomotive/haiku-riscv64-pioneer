#!/usr/bin/env python3
"""Compile exact queued-Q4 role and late/old-package refusals; NO device access."""
import os,pathlib,re,subprocess
if not __debug__ or os.environ.get('PYTHONOPTIMIZE'):raise SystemExit('Checks required')
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
here=pathlib.Path(__file__).resolve().parent
kernel=here.parents[2]/'src/add-ons/kernel/drivers/misc'
role=kernel/'PioneerQ38FullOffloadRole.h'
own=kernel/'PioneerQ38FullOffloadQueuedQ4Role.h'
selected=here.parent/'PioneerQ38TokenSelectedPrograms.h'
required=set(re.findall(r'!defined\(([A-Z_][A-Z0-9_]*)\)\s*\|\|\s*\1\s*!=\s*1\b',role.read_text()))
assert len(required)==23
features=['BATCHED','METRICS','PREPARED_Q4','COUNTER_UNROLL','PHASE_PROFILE','TWO_PRODUCT_REPLAY','INSTRUCTION_PREFETCH','QUEUED_Q4']
features=['PIONEER_Q38_FULL_OFFLOAD_'+k for k in features]
base={k:'1' for k in required|set(features)}
base['PIONEER_DRIVER_BUILD_TAG']='0x2026110500010102ULL'
include=lambda p:'#include "'+str(p)+'"\n'
positive=negative=0
def check(values,okay,body):
 global positive,negative
 source=''.join('#define '+k+' '+v+'\n' for k,v in sorted(values.items()))+body
 result=subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Werror','-fsyntax-only','-x','c++','-'],
                       input=source,text=True,capture_output=True)
 assert (result.returncode==0)==okay,(values,body,result.stderr)
 if okay:positive+=1
 else:
  assert 'error:' in result.stderr;negative+=1
check(base,True,include(role)+include(role))
check(base,True,include(selected)+include(selected)+
 'static_assert(PioneerQ38FullOffloadFirmwareSelection==10 && PioneerQ38TokenPrograms::QueuedQ4Block==1 && PioneerQ38TokenPrograms::QueuedQ4Wait==0xa2ff9816U && PioneerQ38TokenPrograms::InstructionPrefetch==1 && PioneerQ38TokenPrograms::PrefetchLastByte==0xc0c67);\n')
for flag in sorted(required|set(features)):
 for value in (None,'0','2'):
  bad=base.copy()
  if value is None:del bad[flag]
  else:bad[flag]=value
  check(bad,False,include(role))
for flag in features:
 for value in (None,'0','2'):
  tail='#undef '+flag+'\n'+('' if value is None else '#define '+flag+' '+value+'\n')
  check(base,False,include(selected)+tail+include(selected))
for tag in ('0x2026110500010101ULL','0x2026110500010100ULL','0x2026110500010099ULL','0','0x2026102100010012ULL'):
 check({**base,'PIONEER_DRIVER_BUILD_TAG':tag},False,include(role))
for flag in ('PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_FULL_MODEL_CPU_REFERENCE','PIONEER_Q38_WORK_INFERENCE',
             'TT_PIONEER_Q38_WORK_MODE','PIONEER_Q38_FULL_OFFLOAD_CLIENT','PIONEER_Q38_INSTRUCTION_PREFETCH_WORKER_IMAGE',
             'PIONEER_Q38_FULL_OFFLOAD_ROOT_IMAGE','PIONEER_Q38_FULL_OFFLOAD_RESIDENT','PIONEER_Q38_QUEUED_Q4_BLOCK_IMAGE','PIONEER_Q38_QUEUED_Q4_BLOCK_WORKER_TEST'):
 for value in ('0','1'):check({**base,flag:value},False,include(role))
for package in ('InstructionPrefetch','TwoProductReplay','PreparedProfile','PreparedCounter','Prepared','Macro','Vector'):
 old=here.parent/('PioneerQ38FullOffload'+package+'Q4Programs.h')
 check(base,False,include(old)+include(selected))
 check(base,False,include(selected)+include(old))
user={k:'1' for k in features};user['PIONEER_Q38_FULL_OFFLOAD_CLIENT']='1'
check(user,True,include(own)+include(own)+'#define B_DEVICE_OP_CODES_END 1000\n'+
 include(kernel/'PioneerQ38FullOffloadProtocol.h')+'static_assert(PioneerQ38FullOffload::BuildTag==UINT64_C(0x2026110500010102));\n')
for flag in features+['PIONEER_Q38_FULL_OFFLOAD_CLIENT']:
 for value in (None,'0','2'):
  bad=user.copy()
  if value is None:del bad[flag]
  else:bad[flag]=value
  check(bad,False,include(own))
print(f'PASS queued-Q4 exact tag102/selection10 native+USER: {positive} positives/{negative} missing,zero,two,late,parent,old-package,CPU/Work refusals;NO device or speed result',flush=True)
