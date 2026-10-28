#!/usr/bin/env python3
"""No CPU source/model/reference boundary may be included by a device role."""
import os
import pathlib
import re
import subprocess
here=pathlib.Path(__file__).resolve().parent
header=here.parent/'PioneerQ38RecurrentPrefixOriginalSource.h'
parent=here.parent/'PioneerQ38RecurrentBlockOriginalSource.h'
include='#include "'+str(header)+'"\n';cc=os.environ.get('CXX','c++');cases=0
def gate(text,flags,okay):
 p=subprocess.run([cc,'-std=c++17','-x','c++','-E','-','-o',os.devnull,*flags],input=text,text=True,capture_output=True)
 assert (p.returncode==0)==okay,p.stderr
gate(include+include,[],True)
for alias in sorted(set(re.findall(r'defined\(([^)]+)\)',header.read_text()+parent.read_text()))):
 for value in ('0','1'):
  gate(include,['-D'+alias+'='+value],False)
  gate(include+'#define '+alias+' '+value+'\n'+include,[],False);cases+=2
print('PASS prefix original CPU source boundary '+str(cases)+' image/native/client/Work/kernel defined0+late refusals;NO numerical products exposed')
