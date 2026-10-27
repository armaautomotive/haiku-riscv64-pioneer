#!/usr/bin/env python3
"""Separate diagnostic client role/CLI builder,never a native or Work grant."""
import os
import pathlib
import re
import subprocess
import sys
import tempfile
here=pathlib.Path(__file__).resolve().parent;header=here.parent/'PioneerQ38RecurrentBlockValidationSession.h'
body=header.read_text();parents={'PIONEER_Q38_RECURRENT_BLOCK_VALIDATION_CLIENT','PIONEER_Q38_CPU_VALIDATION'}
forbidden=set(re.findall(r'(?<!!)defined\(([A-Z_][A-Z0-9_]*)\)',body))-parents
assert {'_KERNEL_MODE','PIONEER_Q38_GDN_MUTABLE_STATE','PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE',
 'PIONEER_Q38_RECURRENT_BLOCK_VALIDATION_REFERENCE','PIONEER_Q38_RECURRENT_BLOCK_VALIDATION_NATIVE'}<=forbidden
clean={k:v for k,v in os.environ.items() if not k.startswith(('PIONEER_','TT_PIONEER_'))};count=0
include='#include "'+str(header)+'"\n';valid={key:'1' for key in parents}
def check(defines,okay,source=include):
 global count
 result=subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-E','-P','-x','c++','-DB_DEVICE_OP_CODES_END=1000',
  *['-D'+k+'='+v for k,v in sorted(defines.items())],'-'],input=source,text=True,stdout=subprocess.DEVNULL,stderr=subprocess.PIPE,env=clean,timeout=10)
 assert (result.returncode==0)==okay,(defines,result.stderr);count+=1
check(valid,True);check(valid,True,include+include)
for key in parents:
 for value in (None,'0','2','-1',''):
  altered={k:v for k,v in valid.items() if k!=key}
  if value is not None:altered[key]=value
  check(altered,False)
  check(valid,False,include+'#undef '+key+'\n'+('' if value is None else '#define '+key+' '+value+'\n')+include)
for key in sorted(forbidden):
 for value in ('0','1'):
  check({**valid,key:value},False)
  check(valid,False,include+'#define '+key+' '+value+'\n'+include)
with tempfile.TemporaryDirectory(prefix='pioneer-qrb-client-gates-',dir='/private/tmp') as tmp:
 out=pathlib.Path(tmp)/'NO_OUTPUT';builder=here.parent/'build_qwen38_recurrent_block_validation_client.py'
 for key in ('PIONEER_Q38_RECURRENT_BLOCK_VALIDATION_CLIENT','PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_WORK_INFERENCE'):
  for value in ('0','1',''):
   result=subprocess.run([sys.executable,str(builder),str(pathlib.Path(tmp)/'NO_SDK'),str(out)],env={**clean,key:value},capture_output=True,timeout=5)
   assert result.returncode==2 and not result.stdout and not result.stderr and not out.exists();count+=1
print('PASS distinct DEBUG71 CPU-client gates'+str(count)+'/exact2roles/defined0+late/native+mutable-owner+image+inline-private-oracle+old dense+Work refusal/inert builder;NO device/full-model/native authority')
