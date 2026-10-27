#!/usr/bin/env python3
"""Exact private QRBNODRV perimeter;no SDK/device access or native authority."""
import os
import pathlib
import re
import subprocess
import sys
import tempfile
here=pathlib.Path(__file__).resolve().parent;project=here.parents[2];kernel=project/'src/add-ons/kernel/drivers/misc'
role=kernel/'PioneerQ38RecurrentBlockNativeRole.h';body=role.read_text()
parents=set(re.findall(r'!defined\(([A-Z][A-Z0-9_]*)\)\s*\|\|\s*\1\s*!=\s*1\b',body));assert len(parents)==21
names=('api_version','init_hardware','init_driver','uninit_driver','publish_devices','find_device')
valid={k:'1' for k in parents};valid['PIONEER_DRIVER_BUILD_TAG']='0x5152424e4f445256ULL';valid.update({n:'pioneer_checked_'+n for n in names})
clean={k:v for k,v in os.environ.items() if not k.startswith(('PIONEER_','TT_PIONEER_'))};checks=0
include='#include "'+str(role)+'"\n'
def check(source,defines,okay):
 global checks
 p=subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-E','-x','c++','-I'+str(kernel),*['-D'+k+'='+v for k,v in defines.items()],'-'],
  input=source,text=True,capture_output=True,env=clean,timeout=10)
 assert (p.returncode==0)==okay,(defines,p.stderr);checks+=1
check(include,valid,True);check(include+include,valid,True)
for key in sorted(parents):
 for value in (None,'0','2','-1',''):
  altered={k:v for k,v in valid.items() if k!=key}
  if value is not None:altered[key]=value
  check(include,altered,False)
  late='#undef '+key+'\n'+('' if value is None else '#define '+key+' '+value+'\n')
  check(include+late+include,valid,False)
for key in names:
 altered={k:v for k,v in valid.items() if k!=key};check(include,altered,False)
 check(include+'#undef '+key+'\n'+include,valid,False)
for tag in ('0','0x2026102700010070ULL','0x51524c4e4f445256ULL','0x51424c4e4f445256ULL','0x2026102100010012ULL'):
 check(include,{**valid,'PIONEER_DRIVER_BUILD_TAG':tag},False)
 check(include+'#undef PIONEER_DRIVER_BUILD_TAG\n#define PIONEER_DRIVER_BUILD_TAG '+tag+'\n'+include,valid,False)
aliases=set(re.findall(r'defined\(([^)]+)\)',body))-parents-{'PIONEER_DRIVER_BUILD_TAG',*names}
for key in sorted(aliases):
 for value in ('0','1'):
  check(include,{**valid,key:value},False);check(include+'#define '+key+' '+value+'\n'+include,valid,False)
# Prefix-only preprocessor gates prove no partial role/witness or reserved
# tag can select the production context before real kernel headers load.
prefix=(kernel/'tenstorrent_pci_probe.cpp').read_text().split('#include "tenstorrent_pci_probe.h"',1)[0]
check(prefix,{},True);check(prefix,valid,True)
for value in ('0','1'):
 check(prefix,{'PIONEER_Q38_RECURRENT_BLOCK_NATIVE_CONTEXT':value},False)
 check(prefix,{**valid,'PIONEER_Q38_RECURRENT_BLOCK_NATIVE_ROLE_TAG_OK':value},False)
check(prefix,{'TT_PIONEER_DRIVER_RELOAD':'1','PIONEER_DRIVER_BUILD_TAG':'0x5152424e4f445256ULL'},False)
with tempfile.TemporaryDirectory(prefix='pioneer-qrb-native-role-',dir='/private/tmp') as tmp:
 out=pathlib.Path(tmp)/'NO_OUTPUT';builder=here/'run_pioneer_q38_recurrent_block_native_context_compile.py'
 for key in ('PIONEER_Q38_RECURRENT_BLOCK_NATIVE_CONTEXT','PIONEER_Q38_CPU_VALIDATION','TT_PIONEER_Q38_WORK_MODE'):
  for value in ('0','1',''):
   p=subprocess.run([sys.executable,str(builder),str(pathlib.Path(tmp)/'NO_SDK'),str(out)],env={**clean,key:value},capture_output=True,timeout=5)
   assert p.returncode==2 and not p.stdout and not p.stderr and not out.exists();checks+=1
print('PASS distinct QRBNODRV exact-role gates'+str(checks)+'/21parents/ALL6renamed ABI/late+defined0+reservedtag+caller-witness refusal/inert builder preflight;NO link/install/device/Work authority')
