#!/usr/bin/env python3
"""Exact physical70 role/unchanged NODRV boundary;preprocessing ONLY."""
import os
import pathlib
import re
import subprocess
kernel=pathlib.Path(__file__).resolve().parents[3]/'src/add-ons/kernel/drivers/misc'
role=kernel/'PioneerQ38MixedLiveValidationRole.h'
text=role.read_text()
parents=set(re.findall(r'!defined\(([A-Z][A-Z0-9_]*)\)\s*\|\|\s*\1\s*!=\s*1\b',text));assert len(parents)==23
tag='PIONEER_DRIVER_BUILD_TAG'
forbidden=set(re.findall(r'(?<!!)defined\(([A-Za-z][A-Za-z0-9_]*)\)',text))-parents-{tag}
base={k:'1' for k in parents};base[tag]='0x2026102700010070ULL'
count=0
def check(defines,okay,late='',header=role):
 global count
 source=''.join('#define '+k+' '+v+'\n' for k,v in sorted(defines.items()))+'#include "'+str(header)+'"\n'+late
 r=subprocess.run([os.environ.get('CXX','c++'),'-E','-P','-x','c++','-'],input=source,text=True,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
 assert (r.returncode==0)==okay,(defines,late,r.stderr);count+=1
check(base,True)
check(base,True,header=kernel/'PioneerQ38MixedLiveNativeRole.h')
check(base,True,header=kernel/'PioneerQ38MixedLiveFinalReleaseRole.h')
for key in parents|{tag}:
 d=dict(base);d.pop(key);check(d,False)
 for value in ('0','2'):
  d=dict(base);d[key]=value;check(d,False)
 check(base,False,'#undef '+key+'\n#define '+key+' 0\n#include "'+str(role)+'"\n')
for key in forbidden:
 for value in ('0','1'):
  d=dict(base);d[key]=value;check(d,False)
 check(base,False,'#define '+key+' 0\n#include "'+str(role)+'"\n')
for value in ('0x51424c4e4f445256ULL','0x2026102600010069ULL','0x2026102500010041ULL'):
 d=dict(base);d[tag]=value;check(d,False)
print('PASS DEBUG70 exact role gates'+str(count)+'/23parents/conventional ABI only/NODRV+old69+CPU+Work+images refused/defined0+late')
