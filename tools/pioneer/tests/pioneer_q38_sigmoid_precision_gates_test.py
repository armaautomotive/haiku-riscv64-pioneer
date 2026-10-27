#!/usr/bin/env python3
"""New sigmoid arithmetic is not an old/native/CPU/Work role alias."""
import os
import pathlib
import re
import subprocess
here=pathlib.Path(__file__).resolve().parent
header=here.parent/'PioneerQ38SigmoidPrecision.h'
prefix=header.read_text().split('#ifndef PIONEER_Q38_SIGMOID_PRECISION_H',1)[0]
key='PIONEER_Q38_SIGMOID_PRECISION_IMAGE'
valid={key:'1'}
count=0
def check(flags,expected,source=prefix):
 global count
 result=subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-E','-x','c++',
  *['-D'+k+'='+v for k,v in flags.items()],'-'],input=source,text=True,stdout=subprocess.DEVNULL,stderr=subprocess.PIPE,timeout=30)
 assert (result.returncode==0)==expected,(flags,result.stderr)
 count+=1
check(valid,True)
check({},False)
for value in ('0','2','-1'):check({key:value},False)
include='#include "'+str(header)+'"\n'
check(valid,True,include)
for forbidden in sorted(set(re.findall(r'defined\(([^)]+)\)',prefix))-{key}):
 for value in ('0','1'):
  check({**valid,forbidden:value},False)
  check(valid,False,include+'#define '+forbidden+' '+value+'\n'+include)
print('PASS isolated sigmoid role gates'+str(count)+'/exact missing defined0 late mixed CPU/oldSiLU/native/kernel/Work;NO capability alias')
