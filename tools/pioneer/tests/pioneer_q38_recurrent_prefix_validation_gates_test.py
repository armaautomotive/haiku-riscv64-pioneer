#!/usr/bin/env python3
"""Separate diagnostic client/source interface; NEVER a native selector."""
import os
import pathlib
import re
import subprocess
here=pathlib.Path(__file__).resolve().parent;root=here.parent
path=root/'PioneerQ38RecurrentPrefixValidationSession.h'
head=path.read_text().split('#ifndef PIONEER_Q38_RECURRENT_PREFIX_VALIDATION_SESSION_H',1)[0]
parents=set(re.findall(r'!defined\(([A-Z][A-Z0-9_]*)\)\s*\|\|\s*\1\s*!=\s*1\b',head));assert len(parents)==2
forbidden=set(re.findall(r'(?<!!)defined\(([A-Z_][A-Z0-9_]*)\)',head))-parents
assert {'PIONEER_Q38_RECURRENT_PREFIX_NATIVE_CONTEXT','PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE'}<=forbidden
clean={k:v for k,v in os.environ.items() if not k.startswith(('PIONEER_','TT_PIONEER_'))}
include='#include "'+str(path)+'"\n';base={k:'1' for k in parents};base['B_DEVICE_OP_CODES_END']='1000';count=0
def check(defines,accept,body=None):
 global count
 source=''.join('#define '+k+' '+v+'\n' for k,v in sorted(defines.items()))+(include if body is None else body)
 r=subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Werror','-E','-x','c++','-'],input=source,text=True,
  stdout=subprocess.DEVNULL,stderr=subprocess.PIPE,env=clean,timeout=30)
 assert (r.returncode==0)==accept,(defines,body,r.stderr);count+=1
check(base,True)
for key in sorted(parents):
 missing=dict(base);del missing[key];check(missing,False)
 for value in ('0','2','-1',''):
  check({**base,key:value},False);check(base,False,include+'#undef '+key+'\n#define '+key+' '+value+'\n'+include)
for key in sorted(forbidden):
 for value in ('0','1'):
  check({**base,key:value},False);check(base,False,include+'#define '+key+' '+value+'\n'+include)
actual='#include "'+str(root/'PioneerQ38RecurrentPrefixOriginalSource.h')+'"\n'
check(base,False,actual);check({},True,actual)
adapter=root/'pioneer_q38_recurrent_prefix_validation_source.cpp';adapter_include='#include "'+str(adapter)+'"\n'
check({},True,adapter_include)
for key in parents|forbidden|{'PIONEER_Q38_RECURRENT_BLOCK_VALIDATION_CLIENT'}:
 for value in ('0','1'):check({key:value},False,adapter_include)
print('PASS DISTINCT prefix client/source gates'+str(count)+'/2 exact client+CPU parents/defined0+late native,image,old-client,Work refusal/actual private CPU source+adapter forbidden in client TU;NO driver or physical authority')
