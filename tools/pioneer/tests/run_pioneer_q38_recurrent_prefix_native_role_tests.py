#!/usr/bin/env python3
"""Compile-perimeter refusals only; never a native execution/driver test."""
import os
import pathlib
import re
import subprocess
here=pathlib.Path(__file__).resolve().parent
path=here.parents[2]/'src/add-ons/kernel/drivers/misc/PioneerQ38RecurrentPrefixNativeRole.h'
role=path.read_text()
parents=set(re.findall(r'!defined\(([A-Z][A-Z0-9_]*)\)\s*\|\|\s*\1\s*!=\s*1\b',role))
assert len(parents)==21
abi=('api_version','init_hardware','init_driver','uninit_driver','publish_devices','find_device')
forbidden=set(re.findall(r'defined\(([A-Z_][A-Z0-9_]*)\)',role[role.index('#if defined(PIONEER_Q38_MIXED_LIVE_NATIVE_CONTEXT)'):]))
assert not parents&forbidden and len(forbidden)==57
base={**dict.fromkeys(parents,'1'),'PIONEER_DRIVER_BUILD_TAG':'0x5152504e4f445256ULL',
      **{name:'pioneer_q38_recurrent_prefix_context_body_'+name for name in abi}}
include='#include "'+str(path)+'"\n'
def check(definitions,accept,tail=''):
 text=''.join('#define '+name+' '+value+'\n' for name,value in sorted(definitions.items()))+include+tail
 result=subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Werror','-E','-x','c++','-'],input=text,
                       text=True,stdout=subprocess.DEVNULL,stderr=subprocess.PIPE)
 assert (result.returncode==0)==accept,(accept,result.stderr)
check(base,True);refused=0
for feature in ('PIONEER_Q38_RECURRENT_PREFIX_FINAL_RELEASE','PIONEER_Q38_RECURRENT_PREFIX_VALIDATION_READBACK'):
 check({**base,feature:'1'},True);check(base,True,'#define '+feature+' 1\n'+include)
 for value in ('0','2'):
  check({**base,feature:value},False);check(base,False,'#define '+feature+' '+value+'\n'+include);refused+=2
for name in sorted(parents):
 missing=dict(base);del missing[name];check(missing,False);refused+=1
 for value in ('0','2'):
  changed={**base,name:value};check(changed,False);refused+=1
 check(base,False,'#undef '+name+'\n#define '+name+' 0\n'+include);refused+=1
for name in abi:
 missing=dict(base);del missing[name];check(missing,False);refused+=1
for tag in ('0','0x5152424e4f445256ULL','0x2026102700010072ULL','0x2026102100010012ULL'):
 check({**base,'PIONEER_DRIVER_BUILD_TAG':tag},False);refused+=1
for name in sorted(forbidden):
 for value in ('0','1'):
  check({**base,name:value},False);check(base,False,'#define '+name+' '+value+'\n'+include);refused+=2
print('PASS distinct QRPNODRV native compile perimeter/21 exact parents/6 renamed ABI names/'+str(len(forbidden))+
      ' old-native+CPU+image+Work aliases/late+defined0 refusals'+str(refused)+';NO runtime,link,install or physical authority')
