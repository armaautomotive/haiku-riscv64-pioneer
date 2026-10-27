#!/usr/bin/env python3
"""QRC1 is an offline image, never a native/Work/CPU capability."""
import os
import pathlib
import re
import subprocess
import sys
import tempfile
here=pathlib.Path(__file__).resolve().parents[1]
kernel=here.parents[1]/'src/add-ons/kernel/drivers/misc'
root='PIONEER_Q38_MIXED_RECURRENT_COHORT_ROOT_IMAGE'
capsule='PIONEER_Q38_MIXED_ROOT_CAPSULE_IMAGE'
witness='PIONEER_Q38_MIXED_ROOT_SEAL_BOUND'
role=here/'PioneerQ38MixedRecurrentCohortRootImageRole.h'
clean={k:v for k,v in os.environ.items() if not k.startswith('PIONEER_')}
checks=0
def check(text,defines,okay):
 global checks
 r=subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-E','-x','c++',
  *['-D'+k+'='+v for k,v in defines.items()],'-'],input=text,text=True,capture_output=True,env=clean,timeout=10)
 assert (r.returncode==0)==okay,(defines,text,r.stderr)
 checks+=1
def include(path):return '#include "'+str(path)+'"\n'
def requirements(path,seen=None):
 seen=set() if seen is None else seen
 if path in seen:return {}
 seen.add(path);text=path.read_text()
 valid={k:'1' for k in re.findall(r'!defined\(([A-Z_][A-Z0-9_]*)\)\s*\|\|\s*\1\s*!=\s*1',text)}
 for name in re.findall(r'#include "([^"]+ImageRole\.h)"',text):valid.update(requirements(path.parent/name,seen))
 return valid
inc=include(role)
check(inc,{root:'1'},True);check(inc+inc,{root:'1'},True)
check(inc,{root:'1',capsule:'1'},True)
for v in (None,'0','2','-1',''):
 check(inc,{} if v is None else {root:v},False)
 if v is not None:check(inc+'#undef '+root+'\n#define '+root+' '+v+'\n'+inc,{root:'1'},False)
for name in sorted(set(re.findall(r'defined\(([^)]+)\)',role.read_text()))-{root,capsule}):
 for v in ('0','1'):
  check(inc,{root:'1',name:v},False);check(inc+'#define '+name+' '+v+'\n'+inc,{root:'1'},False)
for v in ('0','2','-1',''):check(inc,{root:'1',capsule:v},False)
hook=include(here/'PioneerQ38MixedCapsuleHook.h')
check(hook,{},True);check(hook,{'PIONEER_Q38_MIXED_CAPSULE_IMAGE':'1'},True)
valid={root:'1',capsule:'1'}
check(inc+hook+hook,valid,True)
check(inc+hook,{root:'1'},False)
check(hook,{capsule:'1'},False)
check(hook,{**valid,witness:'1'},False)
for flag in (root,capsule):
 for v in ('0','2','-1',''):
  check(hook,{**valid,flag:v},False)
  check(hook+'#undef '+flag+'\n#define '+flag+' '+v+'\n'+hook,valid,False)
 check(hook+'#undef '+flag+'\n'+hook,valid,False)
for name in ('PIONEER_Q38_MIXED_CAPSULE_IMAGE','_KERNEL_MODE','PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_WORK_INFERENCE','__FAST_MATH__'):
 for v in ('0','1'):
  check(hook,{**valid,name:v},False)
  check(hook+'#define '+name+' '+v+'\n'+hook,valid,False)
for prior in ({},{'PIONEER_Q38_MIXED_CAPSULE_IMAGE':'1'}):
 late=''.join('#define '+k+' '+v+'\n' for k,v in valid.items())
 check(hook+late+hook,prior,False)
prefix=(kernel/'tenstorrent_pci_probe.cpp').read_text().split('#include "tenstorrent_pci_probe.h"',1)[0]
check(prefix,{},True)
for flag in (root,capsule):
 for v in ('0','1'):check(prefix,{flag:v},False)
# Each existing role must first pass its own exact valid baseline.
for path in sorted(here.glob('*ImageRole.h')):
 if path==role:continue
 valid=requirements(path)
 assert valid,path
 old=include(path);check(old,valid,True)
 for flag in (root,capsule):
  for v in ('0','1'):
   check(old,{**valid,flag:v},False);check(old+'#define '+flag+' '+v+'\n'+old,valid,False)
for name,parent in (('PioneerQ38ResidentLayerValidationReference.h','PIONEER_Q38_RESIDENT_LAYER_VALIDATION_REFERENCE'),
 ('PioneerQ38ResidentLayerValidationSession.h','PIONEER_Q38_RESIDENT_LAYER_VALIDATION_CLIENT'),
 ('PioneerQ38MixedLiveValidationSession.h','PIONEER_Q38_MIXED_LIVE_VALIDATION_CLIENT'),('PioneerQ38MixedLiveOriginalSource.h',None)):
 text=(here/name).read_text().split('#ifndef ',1)[0]
 valid={parent:'1','PIONEER_Q38_CPU_VALIDATION':'1'} if parent else {}
 check(text,valid,True)
 for flag in (root,capsule):
  for v in ('0','1'):check(text,{**valid,flag:v},False)
scripts=('banks_driver','banks_client','inference','local_image','ffn_graph_image','ffn_matrix_image','ffn_persistent_image',
 'projection_graph_image','projection_matrix_image','projection_persistent_image','resident_layer_validation_client',
 'mixed_live_validation_client','resident_layer_validation_native','resident_layer_link_image','resident_advance_image','resident_continue_image',
 'dense_prepare_image','dense_attention_image','dense_projection_image','dense_partition_image','dense_partition_root_image','mixed_worker_image',
 'mixed_dense_root_image','mixed_projection_root_image','mixed_recurrent_projection_root_image','mixed_recurrent_cohort_image')
with tempfile.TemporaryDirectory(prefix='pioneer-recurrent-cohort-gates-',dir='/private/tmp') as tmp:
 d=pathlib.Path(tmp)
 for flag in (root,capsule):
  for value in ('0','1','2','-1','','false','0\n1'):
   for i,name in enumerate(scripts):
    py=i>=12;script=here/('build_qwen38_'+name+('.py' if py else '.sh'))
    args=[str(d/'NO_SDK'),*([str(d/'NO_LIB')] if name=='inference' else []),str(d/'NO_OUTPUT')]
    r=subprocess.run([sys.executable if py else 'sh',str(script),*args],capture_output=True,text=True,env={**clean,flag:value},timeout=5)
    assert r.returncode==2 and not r.stdout and not r.stderr and not (d/'NO_OUTPUT').exists(),(name,flag,value,r.stderr)
    checks+=1
print('PASS QRC1 ROOT/capsule gates'+str(checks)+'/26builders/valid old baselines/late+defined0+private seal witness refusal;NO native/CPU/Work grant or IO')
