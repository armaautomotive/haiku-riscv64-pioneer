#!/usr/bin/env python3
"""New CPU reference is not a native/image/old DEBUG/Work capability alias."""
import os
import pathlib
import re
import subprocess
here=pathlib.Path(__file__).resolve().parent
header=here.parent/'PioneerQ38MixedLiveValidationReference.h'
prefix=header.read_text().split('#ifndef PIONEER_Q38_MIXED_LIVE_VALIDATION_REFERENCE_H',1)[0]
valid={'PIONEER_Q38_MIXED_LIVE_VALIDATION_REFERENCE':'1','PIONEER_Q38_CPU_VALIDATION':'1'}
count=0
def check(flags,expected,source=prefix):
 global count
 result=subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-x','c++','-E',
  *['-D'+k+'='+v for k,v in flags.items()],'-'],input=source,text=True,stdout=subprocess.DEVNULL,stderr=subprocess.PIPE,timeout=30)
 assert (result.returncode==0)==expected,(flags,result.stderr)
 count+=1
check(valid,True)
check({},False)
for key in valid:
 for value in (None,'0','2','-1'):
  flags=dict(valid)
  if value is None:del flags[key]
  else:flags[key]=value
  check(flags,False)
for key in sorted(set(re.findall(r'defined\(([^)]+)\)',prefix))-set(valid)):
 for value in ('0','1'):check({**valid,key:value},False)
include='#include "'+str(header)+'"\n'
check(valid,True,include)
for key in ('PIONEER_Q38_MIXED_LIVE_NATIVE_CONTEXT','PIONEER_Q38_MIXED_LIVE_VALIDATION_CLIENT','PIONEER_Q38_WORK_INFERENCE','PIONEER_Q38_RESIDENT_LAYER_VALIDATION_REFERENCE'):
 check(valid,False,include+'#define '+key+' 0\n'+include)
interface=here.parent/'PioneerQ38MixedLiveOriginalSource.h'
interface_prefix=interface.read_text().split('#ifndef PIONEER_Q38_MIXED_LIVE_ORIGINAL_SOURCE_H',1)[0]
check({},True,interface_prefix)
for key in sorted(set(re.findall(r'defined\(([^)]+)\)',interface_prefix))):
 for value in ('0','1'):check({key:value},False,interface_prefix)
source_include='#include "'+str(interface)+'"\n'
check({},True,source_include)
check({},False,source_include+'#define PIONEER_Q38_WORK_INFERENCE 0\n'+source_include)
raw_adapter=here/'pioneer_q38_mixed_live_original_bytes.h'
raw_include='#include "'+str(raw_adapter)+'"\n'
check({},True,raw_include)
for key in sorted(set(re.findall(r'defined\(([^)]+)\)',interface_prefix))):
 for value in ('0','1'):
  check({key:value},False,raw_include)
  check({},False,raw_include+'#define '+key+' '+value+'\n'+raw_include)
print('PASS distinct dense ORIGINAL CPU reference gates'+str(count)+'/exact missing defined0 mixed late roles;NO native/image/client/oldDEBUG/Work capability')
