#!/usr/bin/env python3
"""All64 scheduler image-role and canonical package DATA, not device execution."""
import os
import pathlib
import subprocess
import tempfile
here=pathlib.Path(__file__).resolve().parent;role=here.parent/'PioneerQ38LayerChainImageRole.h'
clean={k:v for k,v in os.environ.items() if not k.startswith(('PIONEER_','TT_PIONEER_'))};checks=0
include='#include "'+str(role)+'"\n'
def check(defines,tail,okay):
 global checks
 result=subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Werror','-E','-x','c++','-'],
  input=''.join('#define '+k+' '+v+'\n' for k,v in sorted(defines.items()))+include+tail,text=True,
  stdout=subprocess.DEVNULL,stderr=subprocess.PIPE,env=clean)
 assert (result.returncode==0)==okay,result.stderr;checks+=1
for prefix in (False,True):
 base={'PIONEER_Q38_LAYER_CHAIN_SCHEDULER_IMAGE':'1','PIONEER_Q38_LAYER_CHAIN_LAYERS':'4' if prefix else '64'}
 if prefix:base['PIONEER_Q38_LAYER_CHAIN_PREFIX4_TEST_IMAGE']='1'
 check(base,'',True)
 for key in base:
  for value in (None,'0','2',''):
   changed={k:v for k,v in base.items() if k!=key}
   if value is not None:changed[key]=value
   check(changed,'',False);check(base,'#undef '+key+'\n'+('' if value is None else '#define '+key+' '+value+'\n')+include,False)
 other=dict(base);other['PIONEER_Q38_LAYER_CHAIN_LAYERS']='64' if prefix else '4';check(other,'',False)
 for key in ('_KERNEL_MODE','__FAST_MATH__','PIONEER_DRIVER_BUILD_TAG','TT_PIONEER_DRIVER_RELOAD',
  'PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE',
  'PIONEER_Q38_LAYER_RETURN_COMPONENT_IMAGE','PIONEER_Q38_LAYER_COMPONENT_DENSE_STOP',
  'PIONEER_Q38_LAYER_COMPONENT_RECURRENT','PIONEER_Q38_LAYER_COMPONENT_DENSE','PIONEER_Q38_LAYER_COMPONENT_CHAIN_LAYERS',
  'PIONEER_Q38_MIXED_RECURRENT_BLOCK_ROOT_IMAGE','PIONEER_Q38_MIXED_LIVE_BLOCK_ROOT_IMAGE',
  'PIONEER_Q38_MIXED_BLOCK_ROOT_IMAGE','PIONEER_Q38_MIXED_JOINED_ROOT_IMAGE','PIONEER_Q38_MIXED_PROJECTION_ROOT_IMAGE',
  'PIONEER_Q38_MIXED_RECURRENT_PROJECTION_ROOT_IMAGE','PIONEER_Q38_MIXED_DENSE_ROOT_IMAGE','PIONEER_Q38_MIXED_RECURRENT_COHORT_ROOT_IMAGE',
  'PIONEER_Q38_MIXED_WORKER_IMAGE','PIONEER_Q38_MIXED_CAPSULE_IMAGE','PIONEER_Q38_MIXED_ROOT_CAPSULE_IMAGE',
  'PIONEER_Q38_DENSE_PREPARE_IMAGE','PIONEER_Q38_DENSE_ATTENTION_IMAGE','PIONEER_Q38_DENSE_PROJECTION_IMAGE',
  'PIONEER_Q38_DENSE_PARTITION_IMAGE','PIONEER_Q38_DENSE_PARTITION_ROOT_IMAGE',
  'PIONEER_Q38_RECURRENT_PREFIX_SCHEDULER_IMAGE','PIONEER_Q38_RECURRENT_PREFIX_NATIVE_CONTEXT',
  'PIONEER_Q38_RECURRENT_PREFIX_VALIDATION_NATIVE','PIONEER_Q38_RECURRENT_PREFIX_VALIDATION_CLIENT',
  'PIONEER_Q38_RECURRENT_PREFIX_VALIDATION_REFERENCE','PIONEER_Q38_RECURRENT_PREFIX_VALIDATION_CONTROL_COMPILE_ONLY',
  'PIONEER_Q38_RECURRENT_PREFIX_FROZEN_PLAN'):
  for value in ('0','1'):check({**base,key:value},'',False);check(base,'#define '+key+' '+value+'\n'+include,False)
print('PASS distinct scheduler IMAGE gates'+str(checks)+'/all64 vs explicit prefix4/late+defined0/native+oracle+old-image+Work isolation;NO runtime grant',flush=True)
with tempfile.TemporaryDirectory(prefix='pioneer-layer-chain-data-',dir='/private/tmp') as tmp:
 binary=pathlib.Path(tmp)/'test'
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-ffp-contract=off',
  '-fsanitize=address,undefined',str(here/'pioneer_q38_layer_chain_package_test.cpp'),'-o',str(binary)],check=True,env=clean)
 subprocess.run([str(binary)],check=True,env={**clean,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
