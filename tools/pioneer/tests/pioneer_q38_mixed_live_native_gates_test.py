#!/usr/bin/env python3
"""Separate genuine native-role interlocks,including defined0/reinclusion."""
import os
import pathlib
import re
import subprocess
role=pathlib.Path(__file__).resolve().parents[3]/'src/add-ons/kernel/drivers/misc/PioneerQ38MixedLiveNativeRole.h'
source=role.read_text()
parents=set(re.findall(r'!defined\(([A-Z][A-Z0-9_]*)\)\s*\|\|\s*\1\s*!=\s*1\b',source))
assert len(parents)>=20
abi={'api_version','init_hardware','init_driver','uninit_driver','publish_devices','find_device'}
tag='PIONEER_DRIVER_BUILD_TAG'
forbidden=set(re.findall(r'(?<!!)defined\(([A-Z][A-Z0-9_]*)\)',source))-parents-{tag}
assert {'PIONEER_Q38_WORK_INFERENCE','PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_RESIDENT_LAYER_NATIVE_CONTEXT'}<=forbidden
base={k:'1' for k in parents};base[tag]='0x51424c4e4f445256ULL';base.update({k:'pioneer_q38_live_context_body_'+k for k in abi})
count=0
def check(defines,okay,late=''):
 global count
 text=''.join('#define '+k+' '+v+'\n' for k,v in sorted(defines.items()))+'#include "'+str(role)+'"\n'+late
 result=subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-E','-P','-x','c++','-'],input=text,text=True,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
 assert (result.returncode==0)==okay,(defines,late,result.stderr)
 count+=1
check(base,True)
for key in parents|abi|{tag}:
 d=dict(base);d.pop(key);check(d,False)
 if key not in abi:
  d=dict(base);d[key]='0';check(d,False)
for key in forbidden:
 for value in ('0','1'):
  d=dict(base);d[key]=value;check(d,False)
 check(base,False,'#define '+key+' 0\n#include "'+str(role)+'"\n')
for key in parents:
 check(base,False,'#undef '+key+'\n#define '+key+' 0\n#include "'+str(role)+'"\n')
print('PASS distinct native compile perimeter gates'+str(count)+'/parents'+str(len(parents))+'/all6 ABI names renamed/exact NODRV tag/defined0+late reinclusion/old69+images+CPU+Work refused;NO native execution grant')
