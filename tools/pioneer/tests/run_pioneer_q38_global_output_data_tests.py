#!/usr/bin/env python3
"""Distinct global worker role and canonical output DATA/math, no device IO."""
import os
import pathlib
import re
import subprocess
import sys
import tempfile
here=pathlib.Path(__file__).resolve().parent;root=here.parents[2];role=here.parent/'PioneerQ38GlobalWorkerImageRole.h'
flag='PIONEER_Q38_GLOBAL_WORKER_IMAGE';include='#include "'+str(role)+'"\n';count=0
clean={k:v for k,v in os.environ.items() if not k.startswith(('PIONEER_','TT_PIONEER_'))}
def check(body,defines,okay):
 global count
 r=subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Werror','-E','-x','c++','-I'+str(here.parent),
  '-I'+str(root/'src/add-ons/kernel/drivers/misc'),*['-D'+k+'='+v for k,v in defines.items()],'-'],input=body,text=True,stdout=subprocess.DEVNULL,stderr=subprocess.PIPE,env=clean)
 assert (r.returncode==0)==okay,(defines,r.stderr);count+=1
check(include,{flag:'1'},True);check(include+include,{flag:'1'},True)
for value in (None,'0','2','-1',''):
 check(include,{} if value is None else {flag:value},False)
 check(include+'#undef '+flag+'\n'+('' if value is None else '#define '+flag+' '+value+'\n')+include,{flag:'1'},False)
for key in sorted(set(re.findall(r'defined\(([^)]+)\)',role.read_text()))-{flag}):
 for value in ('0','1'):
  check(include,{flag:'1',key:value},False);check(include+'#define '+key+' '+value+'\n'+include,{flag:'1'},False)
for value in ('0','1'):
 check('#include "PioneerQ38MixedWorkerImageRole.h"\n',{'PIONEER_Q38_MIXED_WORKER_IMAGE':'1',flag:value},False)
 check('#include "PioneerQ38LayerChainImageRole.h"\n',{'PIONEER_Q38_LAYER_CHAIN_SCHEDULER_IMAGE':'1','PIONEER_Q38_LAYER_CHAIN_LAYERS':'64',flag:value},False)
 prefix=(root/'src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp').read_text().split('#include "tenstorrent_pci_probe.h"',1)[0]
 check(prefix,{flag:value},False)
with tempfile.TemporaryDirectory(prefix='pioneer-global-output-data-',dir='/private/tmp') as temp:
 out=pathlib.Path(temp)/'NO_OUTPUT'
 for value in ('0','1','2','-1',''):
  r=subprocess.run([sys.executable,str(here.parent/'build_qwen38_global_worker_image.py'),'/NO_SDK',str(out)],env={**clean,flag:value},capture_output=True,text=True)
  assert r.returncode==2 and not r.stdout and not r.stderr and not out.exists();count+=1
 print('PASS distinct QGW1 IMAGE role gates'+str(count)+'/defined0+late/old mixed+chain+kernel mutual rejection/build refuses env BEFORE output;NO native grant',flush=True)
 binary=pathlib.Path(temp)/'test'
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-ffp-contract=off',
  '-fsanitize=address,undefined',str(here/'pioneer_q38_global_output_data_test.cpp'),'-o',str(binary)],check=True,env=clean)
 subprocess.run([str(binary)],check=True,env={**clean,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
