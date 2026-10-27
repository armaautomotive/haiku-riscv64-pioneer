#!/usr/bin/env python3
"""Readback is an exact private validation selection,not Work/old DEBUG."""
import os
import pathlib
import re
import subprocess
here=pathlib.Path(__file__).resolve().parent
kernel=here.parents[2]/'src/add-ons/kernel/drivers/misc'
role=kernel/'PioneerQ38MixedLiveReadbackRole.h'
parents={'PIONEER_Q38_MIXED_LIVE_VALIDATION_READBACK','PIONEER_Q38_MIXED_LIVE_SIGMOID_PROGRAMS'}
forbidden=set(re.findall(r'(?<!!)defined\(([A-Z_][A-Z0-9_]*)\)',role.read_text()))-parents
assert {'PIONEER_Q38_WORK_INFERENCE','PIONEER_Q38_CPU_VALIDATION','__FAST_MATH__'}<=forbidden
base={k:'1' for k in parents}
count=0
def check(defines,okay,late='',syntax=False):
 global count
 text=''.join('#define '+k+' '+v+'\n' for k,v in sorted(defines.items()))+'#include "'+str(role)+'"\n'+late
 command=[os.environ.get('CXX','c++'),'-std=c++17',*(['-fsyntax-only'] if syntax else ['-E','-P']),'-x','c++','-']
 result=subprocess.run(command,input=text,text=True,stdout=subprocess.DEVNULL,stderr=subprocess.PIPE,timeout=60)
 assert (result.returncode==0)==okay,(defines,late,result.stderr)
 count+=1
check(base,True)
check({**base,'TT_PIONEER_Q38_WORK_MODE':'1'},True) # generic arithmetic option,NOT public Work inference
for key in parents:
 d=dict(base);d.pop(key);check(d,False)
 for value in ('0','2','-1'):
  d=dict(base);d[key]=value;check(d,False)
 check(base,False,'#undef '+key+'\n#define '+key+' 0\n#include "'+str(role)+'"\n')
for key in sorted(forbidden):
 for value in ('0','1'):check({**base,key:value},False)
 check(base,False,'#define '+key+' 0\n#include "'+str(role)+'"\n')
# Real complete owner include twice;no forged include guard or dummy type.
fixture='#define PIONEER_Q38_MIXED_LIVE_OWNER_FIXTURE_ONLY\n#include "'+str(here/'pioneer_q38_mixed_live_owner_test.cpp')+'"\n'
owner='#include "'+str(kernel/'PioneerQ38MixedLiveOwner.h')+'"\n'
check(base,True,fixture+owner,True)
check(base,False,fixture+'#undef PIONEER_Q38_MIXED_LIVE_VALIDATION_READBACK\n'+owner,True)
# The default first include must also reject a late selection.
text='#define PIONEER_Q38_MIXED_LIVE_SIGMOID_PROGRAMS 1\n'+fixture+'#define PIONEER_Q38_MIXED_LIVE_VALIDATION_READBACK 1\n'+owner
result=subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-fsyntax-only','-x','c++','-'],input=text,text=True,stdout=subprocess.DEVNULL,stderr=subprocess.PIPE,timeout=60)
assert result.returncode and 'late readback selection change' in result.stderr,result.stderr
count+=1
print('PASS fixed validation readback role gates'+str(count)+'/exact2 selections/defined0+unknown+late owner changes/CPU+Work+old DEBUG+image aliases rejected;generic arithmetic option is NOT a Work grant')
