#!/usr/bin/env python3
"""Returning layer role and pure DATA tests. Never execute device arithmetic."""
import os
import pathlib
import subprocess
import tempfile
here=pathlib.Path(__file__).resolve().parent;role=here.parent/'PioneerQ38LayerReturnComponentRole.h'
selector='PIONEER_Q38_LAYER_RETURN_COMPONENT_IMAGE';sigmoid='PIONEER_Q38_SIGMOID_PRECISION_IMAGE';checks=0
clean={k:v for k,v in os.environ.items() if not k.startswith(('PIONEER_','TT_PIONEER_'))}
def check(defines,tail,okay):
 global checks
 include='#include "'+str(role)+'"\n'
 text=''.join('#define '+k+' '+v+'\n' for k,v in sorted(defines.items()))+include+tail
 result=subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Werror','-E','-x','c++','-'],input=text,text=True,
  stdout=subprocess.DEVNULL,stderr=subprocess.PIPE,env=clean)
 assert (result.returncode==0)==okay,result.stderr;checks+=1
for family,parent in (('RECURRENT','PIONEER_Q38_MIXED_RECURRENT_BLOCK_ROOT_IMAGE'),('DENSE','PIONEER_Q38_MIXED_LIVE_BLOCK_ROOT_IMAGE')):
 kind='PIONEER_Q38_LAYER_COMPONENT_'+family
 base={selector:'1',kind:'1',parent:'1',sigmoid:'1'};check(base,'',True)
 for required in base:
  for value in (None,'0','2',''):
   changed={k:v for k,v in base.items() if k!=required}
   if value is not None:changed[required]=value
   check(changed,'',False)
   check(base,'#undef '+required+'\n'+('' if value is None else '#define '+required+' '+value+'\n')+'#include "'+str(role)+'"\n',False)
 for forbidden in ('PIONEER_Q38_LAYER_COMPONENT_'+('DENSE' if family=='RECURRENT' else 'RECURRENT'),
  '_KERNEL_MODE','__FAST_MATH__','PIONEER_DRIVER_BUILD_TAG','TT_PIONEER_DRIVER_RELOAD','PIONEER_Q38_CPU_VALIDATION',
  'TT_PIONEER_Q38_WORK_MODE','PIONEER_Q38_WORK_INFERENCE','PIONEER_Q38_RECURRENT_PREFIX_SCHEDULER_IMAGE',
  'PIONEER_Q38_RECURRENT_HISTORY_STAGING_IMAGE','PIONEER_Q38_RECURRENT_PREFIX_NATIVE_CONTEXT',
  'PIONEER_Q38_RECURRENT_PREFIX_VALIDATION_NATIVE','PIONEER_Q38_RECURRENT_PREFIX_VALIDATION_CLIENT',
  'PIONEER_Q38_RECURRENT_PREFIX_VALIDATION_REFERENCE','PIONEER_Q38_RECURRENT_PREFIX_VALIDATION_CONTROL_COMPILE_ONLY',
  'PIONEER_Q38_RECURRENT_PREFIX_FROZEN_PLAN','PIONEER_Q38_RECURRENT_BLOCK_NATIVE_CONTEXT'):
  for value in ('0','1'):
   check({**base,forbidden:value},'',False)
   check(base,'#define '+forbidden+' '+value+'\n#include "'+str(role)+'"\n',False)
print('PASS returning-component role gates'+str(checks)+'/two exact independent families/late+defined0/native+oracle+Work+old-prefix isolation;NO runtime grant',flush=True)
with tempfile.TemporaryDirectory(prefix='pioneer-layer-return-layout-',dir='/private/tmp') as tmp:
 binary=pathlib.Path(tmp)/'test'
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-ffp-contract=off',
  '-fsanitize=address,undefined',str(here/'pioneer_q38_layer_return_component_layout_test.cpp'),'-o',str(binary)],check=True,env=clean)
 subprocess.run([str(binary)],check=True,env={**clean,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
