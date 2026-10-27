#!/usr/bin/env python3
"""Separate CPU-reference algorithm and role isolation;NO model/device IO."""
import os
import pathlib
import re
import subprocess
import tempfile
here=pathlib.Path(__file__).resolve().parent
header=here.parent/'PioneerQ38RecurrentBlockValidationReference.h'
valid={'PIONEER_Q38_RECURRENT_BLOCK_VALIDATION_REFERENCE':'1','PIONEER_Q38_CPU_VALIDATION':'1'}
cc=os.environ.get('CXX','c++')
include='#include "'+str(header)+'"\n'
def gate(text,flags,okay):
 p=subprocess.run([cc,'-std=c++17','-x','c++','-E','-','-o',os.devnull,*['-D'+k+'='+v for k,v in flags.items()]],input=text,text=True,capture_output=True)
 assert (p.returncode==0)==okay,p.stderr
gate(include+include,valid,True);refused=0
for key in valid:
 for value in (None,'0','2',''):
  flags={k:v for k,v in valid.items() if k!=key}
  if value is not None:flags[key]=value
  gate(include,flags,False);refused+=1
  late='#undef '+key+'\n'+('' if value is None else '#define '+key+' '+value+'\n')
  gate(include+late+include,valid,False);refused+=1
aliases=set(re.findall(r'defined\(([^)]+)\)',header.read_text()))-valid.keys()
for key in sorted(aliases):
 for value in ('0','1'):
  gate(include,{**valid,key:value},False);gate(include+'#define '+key+' '+value+'\n'+include,valid,False);refused+=2
with tempfile.TemporaryDirectory(prefix='pioneer-qrb-cpu-reference-',dir='/private/tmp') as temp:
 binary=pathlib.Path(temp)/'test'
 subprocess.run([cc,'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-ffp-contract=off','-fsanitize=address,undefined',
  str(here/'pioneer_q38_recurrent_block_reference_test.cpp'),'-o',str(binary)],check=True)
 subprocess.run([str(binary)],check=True,env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
print('PASS separate recurrent CPU-reference gates'+str(refused)+'/missing+defined0+late/NO old role aliases or native/image/Work authority')
