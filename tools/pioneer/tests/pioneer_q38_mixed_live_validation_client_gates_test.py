#!/usr/bin/env python3
"""Client role boundaries BEFORE repeated include guards;NO device use."""
import os
import pathlib
import re
import subprocess
header=pathlib.Path(__file__).resolve().parents[1]/'PioneerQ38MixedLiveValidationSession.h'
text=header.read_text().split('#ifndef PIONEER_Q38_MIXED_LIVE_VALIDATION_SESSION_H',1)[0]
parents=set(re.findall(r'!defined\(([A-Z][A-Z0-9_]*)\)\s*\|\|\s*\1\s*!=\s*1\b',text));assert len(parents)==2
forbidden=set(re.findall(r'(?<!!)defined\(([A-Za-z_][A-Za-z0-9_]*)\)',text))-parents
base={k:'1' for k in parents};count=0
def check(defines,okay,late=''):
 global count
 source='#define B_DEVICE_OP_CODES_END 1000\n'+''.join('#define '+k+' '+v+'\n' for k,v in sorted(defines.items()))+'#include "'+str(header)+'"\n'+late
 r=subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-E','-P','-x','c++','-'],input=source,text=True,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
 assert (r.returncode==0)==okay,(defines,late,r.stderr);count+=1
check(base,True)
for key in parents:
 d=dict(base);d.pop(key);check(d,False)
 for value in ('0','2'):
  d=dict(base);d[key]=value;check(d,False)
 check(base,False,'#undef '+key+'\n#define '+key+' 0\n#include "'+str(header)+'"\n')
for key in forbidden:
 for value in ('0','1'):
  d=dict(base);d[key]=value;check(d,False)
 check(base,False,'#define '+key+' 0\n#include "'+str(header)+'"\n')
assert '#include "PioneerQ38MixedLiveValidationReference.h"' not in header.read_text()
assert not re.search(r'io\.(Reset|Readback|Write|Retry)',header.read_text())
print('PASS DEBUG70 separate CPU client gates'+str(count)+'/native+Work+kernel+oldDEBUG+images refused/defined0+late;original products remain in separate TU')
