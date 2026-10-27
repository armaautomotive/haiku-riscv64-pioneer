#!/usr/bin/env python3
"""Closed new controller role and valid-baseline old role isolation; no IO."""
import os
import pathlib
import re
import subprocess
import sys
import tempfile
here=pathlib.Path(__file__).resolve().parents[1]
kernel=here.parents[1]/'src/add-ons/kernel/drivers/misc'
flag='PIONEER_Q38_MIXED_RECURRENT_PROJECTION_ROOT_IMAGE';role=here/'PioneerQ38MixedRecurrentProjectionRootImageRole.h'
clean={k:v for k,v in os.environ.items() if not k.startswith('PIONEER_')}
checks=0
def check(text,defines,okay):
 global checks
 r=subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-E','-x','c++',*['-D'+k+'='+v for k,v in defines.items()],'-'],
                  input=text,text=True,capture_output=True,env=clean,timeout=10)
 assert (r.returncode==0)==okay,(defines,r.stderr)
 checks+=1
include='#include "'+str(role)+'"\n'
check(include,{flag:'1'},True);check(include+include,{flag:'1'},True)
for value in (None,'0','2','-1',''):
 check(include,{} if value is None else {flag:value},False)
 if value is not None:check(include+'#undef '+flag+'\n#define '+flag+' '+value+'\n'+include,{flag:'1'},False)
for name in sorted(set(re.findall(r'defined\(([^)]+)\)',role.read_text()))-{flag}):
 for value in ('0','1'):
  check(include,{flag:'1',name:value},False)
  check(include+'#define '+name+' '+value+'\n'+include,{flag:'1'},False)
prefix=(kernel/'tenstorrent_pci_probe.cpp').read_text().split('#include "tenstorrent_pci_probe.h"',1)[0]
check(prefix,{},True)
for v in ('0','1'):check(prefix,{flag:v},False)
for name in ('PioneerQ38MixedProjectionRootImageRole.h','PioneerQ38MixedJoinedRootImageRole.h','PioneerQ38MixedBlockRootImageRole.h','PioneerQ38MixedLiveBlockRootImageRole.h','PioneerQ38DensePrepareImageRole.h','PioneerQ38DenseAttentionImageRole.h','PioneerQ38DenseProjectionImageRole.h',
 'PioneerQ38DensePartitionImageRole.h','PioneerQ38MixedDenseRootImageRole.h','PioneerQ38MixedWorkerImageRole.h','PioneerQ38ResidentContinueImageRole.h',
 'PioneerQ38ResidentAdvanceImageRole.h','PioneerQ38ResidentLayerLinkImageRole.h','PioneerQ38ResidentOperatorImageRole.h'):
 path=here/name;text=path.read_text();valid={k:'1' for k in re.findall(r'!defined\(([A-Z_][A-Z0-9_]*)\)\s*\|\|\s*\1\s*!=\s*1',text)}
 old='#include "'+str(path)+'"\n';assert valid,name;check(old,valid,True)
 for v in ('0','1'):
  check(old,{**valid,flag:v},False);check(old+'#define '+flag+' '+v+'\n'+old,valid,False)
for name,parent in (('PioneerQ38ResidentLayerValidationReference.h','PIONEER_Q38_RESIDENT_LAYER_VALIDATION_REFERENCE'),
 ('PioneerQ38ResidentLayerValidationSession.h','PIONEER_Q38_RESIDENT_LAYER_VALIDATION_CLIENT'),
 ('PioneerQ38MixedLiveValidationSession.h','PIONEER_Q38_MIXED_LIVE_VALIDATION_CLIENT')):
 text=(here/name).read_text().split('#ifndef ',1)[0];valid={parent:'1','PIONEER_Q38_CPU_VALIDATION':'1'};check(text,valid,True)
 for v in ('0','1'):check(text,{**valid,flag:v},False)
text=(here/'PioneerQ38MixedLiveOriginalSource.h').read_text().split('#ifndef ',1)[0]
check(text,{},True)
for v in ('0','1'):check(text,{flag:v},False)
scripts=('banks_driver','banks_client','inference','local_image','ffn_graph_image','ffn_matrix_image','ffn_persistent_image',
 'projection_graph_image','projection_matrix_image','projection_persistent_image','resident_layer_validation_client',
 'resident_layer_validation_native','resident_layer_link_image','resident_advance_image','resident_continue_image',
 'dense_prepare_image','dense_attention_image','dense_projection_image','dense_partition_image','dense_partition_root_image','mixed_worker_image','mixed_dense_root_image','mixed_projection_root_image','mixed_recurrent_projection_root_image')
with tempfile.TemporaryDirectory(prefix='pioneer-mixed-root-gates-',dir='/private/tmp') as temporary:
 d=pathlib.Path(temporary)
 for value in ('0','1','2','-1','','false','0\n1'):
  for i,name in enumerate(scripts):
   py=i>=11;script=here/('build_qwen38_'+name+('.py' if py else '.sh'))
   args=[str(d/'NO_SDK'),*([str(d/'NO_LIB')] if name=='inference' else []),str(d/'NO_OUTPUT')]
   r=subprocess.run([sys.executable if py else 'sh',str(script),*args],capture_output=True,text=True,env={**clean,flag:value},timeout=5)
   assert r.returncode==2 and not r.stdout and not r.stderr and not (d/'NO_OUTPUT').exists(),(name,value,r.stderr)
   checks+=1
print('PASS recurrent projection ROOT gates'+str(checks)+'/24builders/valid old-role baselines BEFORE defined0+late refusal/native-CPU-Work ownership never promoted;NO device IO')
