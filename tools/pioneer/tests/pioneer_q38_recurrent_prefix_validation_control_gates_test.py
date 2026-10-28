#!/usr/bin/env python3
"""Actual control is NODRV compile/test-only;no physical/Work selection."""
import os
import pathlib
import re
import subprocess
here=pathlib.Path(__file__).resolve().parent;kernel=here.parents[2]/'src/add-ons/kernel/drivers/misc'
control=kernel/'PioneerQ38RecurrentPrefixValidationControlRole.h';native=kernel/'PioneerQ38RecurrentPrefixNativeRole.h'
texts=[native.read_text(),control.read_text()]
parents=set().union(*(set(re.findall(r'!defined\(([A-Z][A-Z0-9_]*)\)\s*\|\|\s*\1\s*!=\s*1\b',text)) for text in texts))
assert len(parents)==24
abi=('api_version','init_hardware','init_driver','uninit_driver','publish_devices','find_device')
forbidden=set(re.findall(r'(?<!!)defined\(([A-Z_][A-Z0-9_]*)\)',texts[0][texts[0].index('#if defined(PIONEER_Q38_MIXED_LIVE_NATIVE_CONTEXT)'):]+texts[1]))-parents
base={**dict.fromkeys(parents,'1'),'PIONEER_DRIVER_BUILD_TAG':'0x5152504e4f445256ULL',**{k:'checked_'+k for k in abi}}
include='#include "'+str(control)+'"\n';count=0
clean={k:v for k,v in os.environ.items() if not k.startswith(('PIONEER_','TT_PIONEER_'))}
def check(defines,okay,tail=''):
 global count
 source=''.join('#define '+k+' '+v+'\n' for k,v in sorted(defines.items()))+include+tail
 r=subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Werror','-E','-x','c++','-'],input=source,
  text=True,stdout=subprocess.DEVNULL,stderr=subprocess.PIPE,env=clean,timeout=30)
 assert (r.returncode==0)==okay,(defines,tail,r.stderr);count+=1
check(base,True)
for key in sorted(parents):
 missing=dict(base);del missing[key];check(missing,False)
 for value in ('0','2','-1',''):
  check({**base,key:value},False);check(base,False,'#undef '+key+'\n#define '+key+' '+value+'\n'+include)
for key in sorted(forbidden):
 for value in ('0','1'):
  check({**base,key:value},False);check(base,False,'#define '+key+' '+value+'\n'+include)
for name in abi:
 missing=dict(base);del missing[name];check(missing,False)
for tag in ('0','0x2026102800010073ULL','0x2026102700010072ULL','0x2026102100010012ULL'):
 check({**base,'PIONEER_DRIVER_BUILD_TAG':tag},False)
print('PASS DISTINCT actual prefix control compile-only gates'+str(count)+'/24 exact parents+features/6 renamed ABI names/defined0+late aliases/physical73+old72+stable12 refused;NO normal routing,loadable module,CPU oracle,Work or physical grant')
