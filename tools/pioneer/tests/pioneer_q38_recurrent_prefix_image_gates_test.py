#!/usr/bin/env python3
"""Exact separate offline prefix image; no native or Work promotion."""
import os
import pathlib
import re
import subprocess
here=pathlib.Path(__file__).resolve().parent.parent
role=here/'PioneerQ38RecurrentPrefixImageRole.h'
parents=[here/'PioneerQ38RecurrentHistoryImageRole.h',here/'PioneerQ38MixedRecurrentBlockRootImageRole.h']
required={'PIONEER_Q38_RECURRENT_PREFIX_SCHEDULER_IMAGE','PIONEER_Q38_RECURRENT_HISTORY_STAGING_IMAGE',
          'PIONEER_Q38_MIXED_RECURRENT_BLOCK_ROOT_IMAGE','PIONEER_Q38_SIGMOID_PRECISION_IMAGE'}
allowed='PIONEER_Q38_MIXED_ROOT_CAPSULE_IMAGE'
forbidden=set(re.findall(r'\bdefined\(([A-Z][A-Z0-9_]*)\)',role.read_text()+''.join(p.read_text() for p in parents)))-required-{allowed}
assert len(forbidden)>=58
cases=0
def check(defines,okay,suffix=''):
 global cases
 result=subprocess.run([os.environ.get('CXX','c++'),'-std=c++11','-E','-x','c++',
  *['-D'+k+'='+str(v) for k,v in sorted(defines.items())],'-'],
  input='#include "'+str(role)+'"\n'+suffix,text=True,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
 assert (result.returncode==0)==okay,(defines,suffix)
 cases+=1
base={key:1 for key in required};check(base,True);check({**base,allowed:1},True)
for key in required:
 absent=dict(base);del absent[key];check(absent,False)
 for value in (0,2,-1):check({**base,key:value},False)
for key in forbidden:
 for value in (0,1):check({**base,key:value},False)
 check(base,False,'#define '+key+' 0\n#include "'+str(role)+'"\n')
check({**base,allowed:0},False)
print(f'PASS prefix image role gates{cases}/required exact4/inherited forbids{len(forbidden)}/defined0+late includes;NO native or Work grant')
