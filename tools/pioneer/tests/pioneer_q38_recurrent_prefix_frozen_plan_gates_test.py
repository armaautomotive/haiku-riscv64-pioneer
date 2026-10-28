#!/usr/bin/env python3
"""Private frozen data perimeter/access checks;NO runtime/device grant."""
import os
import pathlib
import re
import subprocess
here=pathlib.Path(__file__).resolve().parent;kernel=here.parents[2]/'src/add-ons/kernel/drivers/misc'
native=(kernel/'PioneerQ38RecurrentPrefixNativeRole.h').read_text()
parents=set(re.findall(r'!defined\(([A-Z][A-Z0-9_]*)\)\s*\|\|\s*\1\s*!=\s*1\b',native));assert len(parents)==21
abi=('api_version','init_hardware','init_driver','uninit_driver','publish_devices','find_device')
feature='PIONEER_Q38_RECURRENT_PREFIX_FROZEN_PLAN'
base={**dict.fromkeys(parents,'1'),feature:'1','PIONEER_DRIVER_BUILD_TAG':'0x5152504e4f445256ULL',**{k:'frozen_test_'+k for k in abi}}
role=kernel/'PioneerQ38RecurrentPrefixFrozenPlanRole.h';header=kernel/'PioneerQ38RecurrentPrefixFrozenPlan.h';count=0
clean={k:v for k,v in os.environ.items() if not k.startswith(('PIONEER_','TT_PIONEER_'))}
def check(defines,text,okay,syntax=False):
 global count
 source=''.join('#define '+k+' '+v+'\n' for k,v in sorted(defines.items()))+text
 result=subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Werror','-fsyntax-only' if syntax else '-E','-x','c++','-'],input=source,
  text=True,stdout=subprocess.DEVNULL,stderr=subprocess.PIPE,env=clean)
 assert (result.returncode==0)==okay,result.stderr;count+=1
include='#include "'+str(role)+'"\n';check(base,include,True);check(base,include+include,True)
for key in sorted(parents|{feature}):
 for value in (None,'0','2',''):
  changed={k:v for k,v in base.items() if k!=key}
  if value is not None:changed[key]=value
  check(changed,include,False)
  check(base,include+'#undef '+key+'\n'+('' if value is None else '#define '+key+' '+value+'\n')+include,False)
for key in abi:
 changed=dict(base);del changed[key];check(changed,include,False)
for key in ('PIONEER_Q38_RECURRENT_PREFIX_VALIDATION_NATIVE','PIONEER_Q38_RECURRENT_PREFIX_PLAN_MIRROR_PROTOTYPE',
 'PIONEER_Q38_RECURRENT_PREFIX_VALIDATION_CLIENT','PIONEER_Q38_CPU_VALIDATION','TT_PIONEER_Q38_WORK_MODE','PIONEER_Q38_RECURRENT_PREFIX_SCHEDULER_IMAGE'):
 for value in ('0','1'):
  check({**base,key:value},include,False);check(base,include+'#define '+key+' '+value+'\n'+include,False)
for tag in ('0x2026102800010073ULL','0x2026102700010072ULL','0x2026102100010012ULL'):
 check({**base,'PIONEER_DRIVER_BUILD_TAG':tag},include,False)
for value in ('0','1'):
 check({**base,'_KERNEL_MODE':'1','PIONEER_Q38_RECURRENT_PREFIX_FROZEN_REFUSALS_TEST':value},include,False)
 check({**base,'_KERNEL_MODE':'1'},include+'#define PIONEER_Q38_RECURRENT_PREFIX_FROZEN_REFUSALS_TEST '+value+'\n'+include,False)
typed='#include "'+str(header)+'"\n#include <type_traits>\n'
check(base,typed+'static_assert(!std::is_default_constructible<PioneerQ38RecurrentPrefixFrozenPlan>::value && !std::is_destructible<PioneerQ38RecurrentPrefixFrozenPlan>::value && !std::is_copy_constructible<PioneerQ38RecurrentPrefixFrozenPlan>::value,"private owner only");\n',True,True)
check(base,typed+'void bad(){PioneerQ38RecurrentPrefixFrozenPlan value;}\n',False,True)
check(base,typed+'bool bad(PioneerQ38RecurrentPrefixFrozenPlan& a,const PioneerQ38CanonicalModelPlan& c,const PioneerQ38RecurrentPrefixPlan& p){return a.Capture(c,p);}\n',False,True)
check(base,typed+'bool bad(PioneerQ38RecurrentPrefixFrozenPlan& a,const PioneerQ38CanonicalModelPlan& c,const PioneerQ38RecurrentPrefixPlan& p){return a.Matches(c,p);}\n',False,True)
print('PASS private frozen-prefix NODRV gates'+str(count)+'/21exact parents+ownflag/6renamedABI/defined0+late/physical+host+client+oracle+image+Work refusals/private ctor+dtor+Capture+Matches;NO native runtime or physical permission')
