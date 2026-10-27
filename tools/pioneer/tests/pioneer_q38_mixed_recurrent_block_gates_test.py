#!/usr/bin/env python3
"""Exact separate QRB1 role, late hook binding and old native/Work isolation."""
import os
import pathlib
import re
import subprocess
import sys
import tempfile
here=pathlib.Path(__file__).resolve().parents[1]
kernel=here.parents[1]/'src/add-ons/kernel/drivers/misc'
flag='PIONEER_Q38_MIXED_RECURRENT_BLOCK_ROOT_IMAGE';precision='PIONEER_Q38_SIGMOID_PRECISION_IMAGE'
capsule='PIONEER_Q38_MIXED_ROOT_CAPSULE_IMAGE';role=here/'PioneerQ38MixedRecurrentBlockRootImageRole.h'
clean={k:v for k,v in os.environ.items() if not k.startswith('PIONEER_')};checks=0
def include(path):return '#include "'+str(path)+'"\n'
def check(text,defines,okay):
 global checks
 p=subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-E','-x','c++',*['-D'+k+'='+v for k,v in defines.items()],'-'],
  input=text,text=True,capture_output=True,env=clean,timeout=10)
 assert (p.returncode==0)==okay,(defines,text,p.stderr)
 checks+=1
def requirements(path,seen=None):
 seen=set() if seen is None else seen
 if path in seen:return {}
 seen.add(path);text=path.read_text();valid={k:'1' for k in re.findall(r'!defined\(([A-Z_][A-Z0-9_]*)\)\s*\|\|\s*\1\s*!=\s*1',text)}
 for name in re.findall(r'#include "([^"]+ImageRole\.h)"',text):valid.update(requirements(path.parent/name,seen))
 return valid
inc=include(role);valid={flag:'1',precision:'1'}
check(inc,valid,True);check(inc+inc,valid,True);check(inc,{**valid,capsule:'1'},True)
for alias in (flag,precision):
 for value in (None,'0','2','-1',''):
  other={k:v for k,v in valid.items() if k!=alias}
  check(inc,other if value is None else {**other,alias:value},False)
  late='#undef '+alias+'\n'+('' if value is None else '#define '+alias+' '+value+'\n')
  check(inc+late+inc,valid,False)
for alias in sorted(set(re.findall(r'defined\(([^)]+)\)',role.read_text()))-{flag,precision,capsule}):
 for value in ('0','1'):
  check(inc,{**valid,alias:value},False);check(inc+'#define '+alias+' '+value+'\n'+inc,valid,False)
hook=include(here/'PioneerQ38MixedCapsuleHook.h');selected={**valid,capsule:'1'}
check(inc+hook+hook,selected,True);check(hook,{flag:'1',capsule:'1'},False);check(hook,valid,False)
for alias in (flag,precision,capsule):
 for value in ('0','2','-1',''):
  check(hook,{**selected,alias:value},False)
  check(hook+'#undef '+alias+'\n#define '+alias+' '+value+'\n'+hook,selected,False)
 check(hook+'#undef '+alias+'\n'+hook,selected,False)
for alias in ('PIONEER_Q38_MIXED_RECURRENT_COHORT_ROOT_IMAGE','PIONEER_Q38_MIXED_CAPSULE_IMAGE','PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_WORK_INFERENCE','_KERNEL_MODE'):
 for value in ('0','1'):check(hook+'#define '+alias+' '+value+'\n'+hook,selected,False)
for prior in ({},{'PIONEER_Q38_MIXED_CAPSULE_IMAGE':'1'}):
 check(hook+''.join('#define '+k+' '+v+'\n' for k,v in selected.items())+hook,prior,False)
for path in sorted(here.glob('*ImageRole.h')):
 if path==role:continue
 old=include(path);baseline=requirements(path);assert baseline,path;check(old,baseline,True)
 for value in ('0','1'):
  check(old,{**baseline,flag:value},False);check(old+'#define '+flag+' '+value+'\n'+old,baseline,False)
prefix=(kernel/'tenstorrent_pci_probe.cpp').read_text().split('#include "tenstorrent_pci_probe.h"',1)[0]
check(prefix,{},True)
for value in ('0','1'):check(prefix,{flag:value},False)
for name,parent in (('PioneerQ38ResidentLayerValidationReference.h','PIONEER_Q38_RESIDENT_LAYER_VALIDATION_REFERENCE'),
 ('PioneerQ38ResidentLayerValidationSession.h','PIONEER_Q38_RESIDENT_LAYER_VALIDATION_CLIENT'),
 ('PioneerQ38MixedLiveValidationSession.h','PIONEER_Q38_MIXED_LIVE_VALIDATION_CLIENT'),('PioneerQ38MixedLiveOriginalSource.h',None)):
 prefix=(here/name).read_text().split('#ifndef ',1)[0];baseline={parent:'1','PIONEER_Q38_CPU_VALIDATION':'1'} if parent else {}
 check(prefix,baseline,True)
 for value in ('0','1'):check(prefix,{**baseline,flag:value},False)
scripts=('banks_driver','banks_client','inference','local_image','ffn_graph_image','ffn_matrix_image','ffn_persistent_image',
 'projection_graph_image','projection_matrix_image','projection_persistent_image','resident_layer_validation_client','mixed_live_validation_client',
 'resident_layer_validation_native','resident_layer_link_image','resident_advance_image','resident_continue_image','dense_prepare_image','dense_attention_image',
 'dense_projection_image','dense_partition_image','dense_partition_root_image','mixed_worker_image','mixed_dense_root_image','mixed_projection_root_image',
 'mixed_recurrent_projection_root_image','mixed_recurrent_cohort_image','mixed_recurrent_block_root_image')
with tempfile.TemporaryDirectory(prefix='pioneer-recurrent-block-gates-',dir='/private/tmp') as tmp:
 d=pathlib.Path(tmp)
 for value in ('0','1','2','-1','','false','0\n1'):
  for i,name in enumerate(scripts):
   py=i>=12;script=here/('build_qwen38_'+name+('.py' if py else '.sh'));args=[str(d/'NO_SDK'),*([str(d/'NO_LIB')] if name=='inference' else []),str(d/'NO_OUTPUT')]
   p=subprocess.run([sys.executable if py else 'sh',str(script),*args],capture_output=True,text=True,env={**clean,flag:value},timeout=5)
   assert p.returncode==2 and not p.stdout and not p.stderr and not (d/'NO_OUTPUT').exists(),(name,value,p.stderr)
   checks+=1
print('PASS QRB1 exact role+precision/private root hook gates'+str(checks)+'/27builders/old valid baselines/late+defined0 refusal;NO native/Work/CPU grant or device IO')
