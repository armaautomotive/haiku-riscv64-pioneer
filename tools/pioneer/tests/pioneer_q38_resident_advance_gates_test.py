#!/usr/bin/env python3
"""New offline device-program role/build isolation. NO execution or hardware."""
import os
import pathlib
import re
import subprocess
import sys
import tempfile
here=pathlib.Path(__file__).resolve().parents[1]
kernel=here.parents[1]/'src/add-ons/kernel/drivers/misc'
flag='PIONEER_Q38_RESIDENT_ADVANCE_IMAGE'
clean={k:v for k,v in os.environ.items() if not k.startswith('PIONEER_')}
role=here/'PioneerQ38ResidentAdvanceImageRole.h';include='#include "'+str(role)+'"\n'
count=0
def check(text,defines,expected):
 global count
 result=subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-E','-x','c++','-I'+str(here),
                        *['-D'+k+'='+v for k,v in defines.items()],'-'],input=text,text=True,capture_output=True,env=clean,timeout=10)
 assert (result.returncode==0)==expected,(defines,result.stderr)
 count+=1
check(include,{flag:'1'},True);check(include+include,{flag:'1'},True)
dispatch=(here/'blackhole_qwen38_dispatch.cpp').read_text().split('typedef __UINT32_TYPE__',1)[0]
matrix={flag:'1','PIONEER_Q38_PROJECTION_MATRIX_ROOT':'1','PIONEER_Q38_PROJECTION_NEXT_ROOT':'1'}
check(dispatch,matrix,True)
for name in ('PIONEER_Q38_PROJECTION_MATRIX_ROOT','PIONEER_Q38_PROJECTION_NEXT_ROOT'):
 for value in (None,'0','2'):
  changed=dict(matrix)
  if value is None:del changed[name]
  else:changed[name]=value
  check(dispatch,changed,False)
 check(dispatch+'#undef '+name+'\n'+dispatch,matrix,False)
for value in (None,'0','2','-1',''):
 check(include,{} if value is None else {flag:value},False)
 if value is not None:check(include+'#undef '+flag+'\n#define '+flag+' '+value+'\n'+include,{flag:'1'},False)
for name in sorted(set(re.findall(r'defined\(([^)]+)\)',role.read_text()))-{flag}):
 for value in ('0','1'):
  check(include,{flag:'1',name:value},False)
  check(include+'#define '+name+' '+value+'\n'+include,{flag:'1'},False)
prefix=(kernel/'tenstorrent_pci_probe.cpp').read_text().split('#include "tenstorrent_pci_probe.h"',1)[0]
for value in ('0','1'):
 check(prefix,{flag:value},False)
 for name in ('PioneerQ38ResidentLayerValidationReference.h','PioneerQ38ResidentLayerValidationSession.h','PioneerQ38ResidentLayerLinkImageRole.h'):
  header=here/name
  text=header.read_text().split('#ifndef ',1)[0]
  check(text,{flag:value,'PIONEER_Q38_RESIDENT_LAYER_LINK_IMAGE':'1','PIONEER_Q38_RESIDENT_LAYER_VALIDATION_CLIENT':'1',
              'PIONEER_Q38_RESIDENT_LAYER_VALIDATION_REFERENCE':'1','PIONEER_Q38_CPU_VALIDATION':'1'},False)
scripts=('banks_driver','banks_client','inference','local_image','ffn_graph_image','ffn_matrix_image','ffn_persistent_image',
         'projection_graph_image','projection_matrix_image','projection_persistent_image','resident_layer_validation_client',
         'resident_layer_validation_native','resident_layer_link_image','resident_advance_image')
with tempfile.TemporaryDirectory(prefix='pioneer-advance-build-gates-',dir='/private/tmp') as temporary:
 d=pathlib.Path(temporary)
 for value in ('0','1','2','-1','','false','0\n1'):
  for name in scripts:
   py=name in ('resident_layer_validation_native','resident_layer_link_image','resident_advance_image')
   script=here/('build_qwen38_'+name+('.py' if py else '.sh'))
   args=[str(d/'NO_SDK'),*([str(d/'NO_LIB')] if name=='inference' else []),str(d/'NO_OUTPUT')]
   result=subprocess.run([sys.executable if py else 'sh',str(script),*args],capture_output=True,text=True,env={**clean,flag:value},timeout=5)
   assert result.returncode==2 and not result.stdout and not result.stderr and not (d/'NO_OUTPUT').exists(),(name,value,result.stderr)
   count+=1
print('PASS advance role/build gates'+str(count)+'/exact offline role/defined0+late old58/69/CPU/Work/component aliases/14 ambient build entries refuse BEFORE SDK/output access;NO native/L1 reservation/device grant')
