#!/usr/bin/env python3
"""NEW continuation build/role containment only; NO hardware."""
import os
import pathlib
import re
import subprocess
import sys
import tempfile
here=pathlib.Path(__file__).resolve().parents[1]
kernel=here.parents[1]/'src/add-ons/kernel/drivers/misc'
flag='PIONEER_Q38_RESIDENT_CONTINUE_IMAGE';role=here/'PioneerQ38ResidentContinueImageRole.h'
clean={k:v for k,v in os.environ.items() if not k.startswith('PIONEER_')}
include='#include "'+str(role)+'"\n';count=0
def check(text,defines,expected):
 global count
 result=subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-E','-x','c++','-I'+str(here),
                        *['-D'+k+'='+v for k,v in defines.items()],'-'],input=text,text=True,capture_output=True,env=clean,timeout=10)
 assert (result.returncode==0)==expected,(defines,result.stderr)
 count+=1
check(include,{flag:'1'},True);check(include+include,{flag:'1'},True)
for value in (None,'0','2','-1',''):
 check(include,{} if value is None else {flag:value},False)
 if value is not None:check(include+'#undef '+flag+'\n#define '+flag+' '+value+'\n'+include,{flag:'1'},False)
for name in sorted(set(re.findall(r'defined\(([^)]+)\)',role.read_text()))-{flag}):
 for value in ('0','1'):
  check(include,{flag:'1',name:value},False)
  check(include+'#define '+name+' '+value+'\n'+include,{flag:'1'},False)
for value in ('0','1'):
 prefix=(kernel/'tenstorrent_pci_probe.cpp').read_text().split('#include "tenstorrent_pci_probe.h"',1)[0]
 check(prefix,{flag:value},False)
 for name in ('PioneerQ38ResidentAdvanceImageRole.h','PioneerQ38ResidentLayerLinkImageRole.h',
              'PioneerQ38ResidentLayerValidationReference.h','PioneerQ38ResidentLayerValidationSession.h'):
  text=(here/name).read_text().split('#ifndef ',1)[0]
  check(text,{flag:value,'PIONEER_Q38_RESIDENT_ADVANCE_IMAGE':'1','PIONEER_Q38_RESIDENT_LAYER_LINK_IMAGE':'1',
              'PIONEER_Q38_RESIDENT_LAYER_VALIDATION_CLIENT':'1','PIONEER_Q38_RESIDENT_LAYER_VALIDATION_REFERENCE':'1','PIONEER_Q38_CPU_VALIDATION':'1'},False)
scripts=('banks_driver','banks_client','inference','local_image','ffn_graph_image','ffn_matrix_image','ffn_persistent_image',
         'projection_graph_image','projection_matrix_image','projection_persistent_image','resident_layer_validation_client',
         'resident_layer_validation_native','resident_layer_link_image','resident_advance_image','resident_continue_image')
with tempfile.TemporaryDirectory(prefix='pioneer-continue-build-gates-',dir='/private/tmp') as temporary:
 d=pathlib.Path(temporary)
 for value in ('0','1','2','-1','','false','0\n1'):
  for name in scripts:
   py=name in ('resident_layer_validation_native','resident_layer_link_image','resident_advance_image','resident_continue_image')
   script=here/('build_qwen38_'+name+('.py' if py else '.sh'))
   args=[str(d/'NO_SDK'),*([str(d/'NO_LIB')] if name=='inference' else []),str(d/'NO_OUTPUT')]
   result=subprocess.run([sys.executable if py else 'sh',str(script),*args],capture_output=True,text=True,env={**clean,flag:value},timeout=5)
   assert result.returncode==2 and not result.stdout and not result.stderr and not (d/'NO_OUTPUT').exists(),(name,value,result.stderr)
   count+=1
print('PASS continuation role/build gates'+str(count)+'/exact offline role/defined0+late native/CPU/Work/old component aliases/15 ambient build entries refuse BEFORE SDK/output access;NO L1/GDDR reservation/device grant')
