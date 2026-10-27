#!/usr/bin/env python3
"""Exact separate physical recurrent diagnostic role;NO build/device use."""
import os
import pathlib
import re
import subprocess
here=pathlib.Path(__file__).resolve().parent
kernel=here.parents[2]/'src/add-ons/kernel/drivers/misc'
role=kernel/'PioneerQ38RecurrentBlockValidationRole.h';body=role.read_text()
parents=set(re.findall(r'!defined\(([A-Z][A-Z0-9_]*)\)\s*\|\|\s*\1\s*!=\s*1\b',body));assert len(parents)==22
names=('api_version','init_hardware','init_driver','uninit_driver','publish_devices','find_device')
valid={k:'1' for k in parents};valid['PIONEER_DRIVER_BUILD_TAG']='0x2026102700010071ULL'
clean={k:v for k,v in os.environ.items() if not k.startswith(('PIONEER_','TT_PIONEER_'))};count=0
include='#include "'+str(role)+'"\n'
def check(source,defines,okay):
 global count
 result=subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-E','-P','-x','c++','-I'+str(kernel),
  *['-D'+k+'='+v for k,v in sorted(defines.items())],'-'],input=source,text=True,capture_output=True,env=clean,timeout=10)
 assert (result.returncode==0)==okay,(defines,result.stderr);count+=1
check(include,valid,True);check(include+include,valid,True)
check(include+'#include "PioneerQ38RecurrentBlockNativeRole.h"\n#include "PioneerQ38RecurrentBlockReadbackRole.h"\n',valid,True)
for key in sorted(parents):
 for value in (None,'0','2','-1',''):
  altered={k:v for k,v in valid.items() if k!=key}
  if value is not None:altered[key]=value
  check(include,altered,False)
  check(include+'#undef '+key+'\n'+('' if value is None else '#define '+key+' '+value+'\n')+include,valid,False)
aliases=set(re.findall(r'defined\(([A-Z][A-Z0-9_]*)\)',body))-parents-{'PIONEER_DRIVER_BUILD_TAG',*names}
for key in sorted(aliases|set(names)):
 for value in ('0','1'):
  check(include,{**valid,key:value},False)
  check(include+'#define '+key+' '+value+'\n'+include,valid,False)
for tag in ('0','0x5152424e4f445256ULL','0x2026102700010070ULL','0x2026102100010012ULL'):
 check(include,{**valid,'PIONEER_DRIVER_BUILD_TAG':tag},False)
 check(include+'#undef PIONEER_DRIVER_BUILD_TAG\n#define PIONEER_DRIVER_BUILD_TAG '+tag+'\n'+include,valid,False)
prefix=(kernel/'tenstorrent_pci_probe.cpp').read_text().split('#include "tenstorrent_pci_probe.h"',1)[0]
check(prefix,{},True);check(prefix,valid,True)
check(prefix,{'TT_PIONEER_DRIVER_RELOAD':'1','PIONEER_DRIVER_BUILD_TAG':'0x2026102700010071ULL'},False)
for value in ('0','1'):
 check(prefix,{'PIONEER_Q38_RECURRENT_BLOCK_VALIDATION_NATIVE':value},False)
 for witness in ('PIONEER_Q38_RECURRENT_BLOCK_PHYSICAL_ROLE_TAG_OK','PIONEER_Q38_RECURRENT_BLOCK_NATIVE_ROLE_TAG_OK'):
  check(prefix,{**valid,witness:value},False)
print('PASS distinct DEBUG71 physical-role gates'+str(count)+'/22exact parents/conventional ABI/defined0+late aliases+reservedtag+caller-witness refusal;NO native runtime/install/card/Work qualification')
