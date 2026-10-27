#!/usr/bin/env python3
"""Additional exact final-release selection:still private QBLNODRV only."""
import os
import pathlib
import re
import subprocess
kernel=pathlib.Path(__file__).resolve().parents[3]/'src/add-ons/kernel/drivers/misc'
role=kernel/'PioneerQ38MixedLiveFinalReleaseRole.h'
end_call=kernel/'PioneerQ38MixedLiveFinalEndCall.inc'
assert end_call.read_text().count('#include "PioneerQ38MixedLiveFinalReleaseRole.h"')==1
control=(kernel/'tenstorrent_pci_probe.cpp').read_text()
assert control.count('#elif defined(PIONEER_Q38_MIXED_LIVE_FINAL_RELEASE)\n#include "PioneerQ38MixedLiveFinalEndCall.inc"')==1
sources='\n'.join((kernel/name).read_text() for name in ('PioneerQ38MixedLiveFinalReleaseRole.h','PioneerQ38MixedLiveNativeRole.h','PioneerQ38MixedLiveReadbackRole.h'))
parents=set(re.findall(r'!defined\(([A-Z][A-Z0-9_]*)\)\s*\|\|\s*\1\s*!=\s*1\b',sources));assert len(parents)==23
abi={'api_version','init_hardware','init_driver','uninit_driver','publish_devices','find_device'}
tag='PIONEER_DRIVER_BUILD_TAG'
forbidden=set(re.findall(r'(?<!!)defined\(([A-Z_][A-Z0-9_]*)\)',sources))-parents-{tag}
# The dedicated header uses #ifdef for its additional old mapping family.
forbidden.add('PIONEER_Q38_PROJECTION_GRAPH_MEMORY')
base={k:'1' for k in parents};base[tag]='0x51424c4e4f445256ULL';base.update({k:'pioneer_final_gate_'+k for k in abi})
count=0
def check(defines,okay,late=''):
 global count
 for target in (role,end_call):
  text=''.join('#define '+k+' '+v+'\n' for k,v in sorted(defines.items()))+'#include "'+str(target)+'"\n'+late.replace(str(role),str(target))
  result=subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-E','-P','-x','c++','-'],input=text,text=True,stdout=subprocess.DEVNULL,stderr=subprocess.PIPE,timeout=30)
  assert (result.returncode==0)==okay,(str(target),defines,late,result.stderr)
  count+=1
check(base,True);check({**base,'TT_PIONEER_Q38_WORK_MODE':'1'},True)
for key in parents|abi|{tag}:
 d=dict(base);d.pop(key);check(d,False)
 if key not in abi:
  for value in ('0','2'):
   d=dict(base);d[key]=value;check(d,False)
for key in sorted(forbidden):
 for value in ('0','1'):check({**base,key:value},False)
 check(base,False,'#define '+key+' 0\n#include "'+str(role)+'"\n')
for key in parents:
 check(base,False,'#undef '+key+'\n#define '+key+' 0\n#include "'+str(role)+'"\n')
for value in ('0x2026102600010069ULL','0x2026102100010012ULL'):
 check({**base,tag:value},False)
print('PASS separate final-release AND actual END-call body gates'+str(count)+'/23exactparentselections/6ABIrenames/exactNODRVtag/defined0+late+oldprojectionmap+CPU+Work+image aliases refused;NO loadable/native-release grant')
