#!/usr/bin/env python3
"""Exact NEW roles,late+defined-zero refusal,old/native mutual exclusion."""
import os
import pathlib
import re
import subprocess
here=pathlib.Path(__file__).resolve().parent.parent
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
root={'PIONEER_Q38_GLOBAL_CHAIN_ROOT_IMAGE':1}
r={'PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE':1,'PIONEER_Q38_SIGMOID_PRECISION_IMAGE':1,'PIONEER_Q38_LAYER_RETURN_COMPONENT_IMAGE':1,'PIONEER_Q38_LAYER_COMPONENT_RECURRENT':1,'PIONEER_Q38_MIXED_RECURRENT_BLOCK_ROOT_IMAGE':1}
d={**r};del d['PIONEER_Q38_LAYER_COMPONENT_RECURRENT'];del d['PIONEER_Q38_MIXED_RECURRENT_BLOCK_ROOT_IMAGE'];d.update(PIONEER_Q38_LAYER_COMPONENT_DENSE=1,PIONEER_Q38_MIXED_LIVE_BLOCK_ROOT_IMAGE=1)
conv={k:v for k,v in r.items() if k not in ('PIONEER_Q38_LAYER_RETURN_COMPONENT_IMAGE','PIONEER_Q38_LAYER_COMPONENT_RECURRENT')};conv.update(PIONEER_Q38_GLOBAL_CHAIN_CONV_CAPSULE_IMAGE=1,PIONEER_Q38_MIXED_ROOT_CAPSULE_IMAGE=1)
gates=0
forbidden=sorted(set(re.findall(r'defined\(([A-Za-z_][A-Za-z0-9_]*)\)',(here/'PioneerQ38GlobalChainIsolation.h').read_text())))
def check(header,flags,okay,late=None):
 global gates
 source=''.join('#define '+k+' '+str(v)+'\n' for k,v in flags.items())+'#include "'+str(here/header)+'"\n'
 if late:source+='#define '+late+' 0\n#include "'+str(here/header)+'"\n'
 result=subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-x','c++','-E','-'],input=source,text=True,capture_output=True)
 assert (result.returncode==0)==okay,(header,flags,late,result.stderr);gates+=1
for header,flags in [('PioneerQ38GlobalChainImageRole.h',root),*[( 'PioneerQ38GlobalChainComponentRole.h',x) for x in (r,d,conv)]]:
 check(header,flags,True)
 for name in flags:
  bad={**flags};bad[name]=0;check(header,bad,False)
  bad={**flags};del bad[name];check(header,bad,False)
 for name in forbidden:
  check(header,{**flags,name:0},False);check(header,flags,False,late=name)
old_roles={
 'PioneerQ38LayerChainImageRole.h':{'PIONEER_Q38_LAYER_CHAIN_SCHEDULER_IMAGE':1,'PIONEER_Q38_LAYER_CHAIN_LAYERS':64},
 'PioneerQ38LayerReturnComponentRole.h':{k:v for k,v in r.items() if k!='PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE'},
 'PioneerQ38MixedWorkerImageRole.h':{'PIONEER_Q38_MIXED_WORKER_IMAGE':1},
 'PioneerQ38GlobalWorkerImageRole.h':{'PIONEER_Q38_GLOBAL_WORKER_IMAGE':1},
 'PioneerQ38MixedRecurrentBlockRootImageRole.h':{'PIONEER_Q38_MIXED_RECURRENT_BLOCK_ROOT_IMAGE':1,'PIONEER_Q38_SIGMOID_PRECISION_IMAGE':1},
 'PioneerQ38MixedLiveBlockRootImageRole.h':{'PIONEER_Q38_MIXED_LIVE_BLOCK_ROOT_IMAGE':1,'PIONEER_Q38_SIGMOID_PRECISION_IMAGE':1}}
for header,flags in old_roles.items():
 check(header,flags,True)
 for name in ('PIONEER_Q38_GLOBAL_CHAIN_ROOT_IMAGE','PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE','PIONEER_Q38_GLOBAL_CHAIN_CONV_CAPSULE_IMAGE'):
  check(header,{**flags,name:0},False)
print('PASS QGC1 exact4 BUILD-only roles/late+defined0+missing flags/old native+worker+chain exclusion/gates'+str(gates)+';NO code,source,native,physical,Work or launch grant')
