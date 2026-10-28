#!/usr/bin/env python3
"""Host-only exact immutable-data mirror experiment;NO driver/device use."""
import os
import pathlib
import re
import subprocess
import tempfile
here=pathlib.Path(__file__).resolve().parent;header=here.parent/'PioneerQ38RecurrentPrefixPlanMirror.h'
body=header.read_text();feature='PIONEER_Q38_RECURRENT_PREFIX_PLAN_MIRROR_PROTOTYPE'
forbidden=set(re.findall(r'defined\(([A-Z_][A-Z0-9_]*)\)',body))- {feature}
clean={k:v for k,v in os.environ.items() if not k.startswith(('PIONEER_','TT_PIONEER_'))};count=0
def preprocess(source,okay):
 global count
 result=subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-E','-x','c++','-'],input=source,text=True,
  stdout=subprocess.DEVNULL,stderr=subprocess.PIPE,env=clean)
 assert (result.returncode==0)==okay,result.stderr;count+=1
include='#include "'+str(header)+'"\n';valid='#define PIONEER_Q38_GDN_MUTABLE_STATE 1\n#define '+feature+' 1\n'
preprocess(valid+include,True);preprocess(valid+include+include,True)
for value in ('','0','2'):
 preprocess('#define PIONEER_Q38_GDN_MUTABLE_STATE 1\n#define '+feature+' '+value+'\n'+include,False)
 preprocess(valid+include+'#undef '+feature+'\n#define '+feature+' '+value+'\n'+include,False)
preprocess('#define PIONEER_Q38_GDN_MUTABLE_STATE 1\n'+include,False)
for name in sorted(forbidden):
 for value in ('0','1'):
  define='#define '+name+' '+value+'\n'
  preprocess(valid+define+include,False);preprocess(valid+include+define+include,False)
print('PASS HOST-ONLY exact mirror role gates'+str(count)+'/native+kernel+client+oracle+image+Work defined0+late refusal;NO production integration',flush=True)
with tempfile.TemporaryDirectory(prefix='pioneer-prefix-mirror-',dir='/private/tmp') as temporary:
 binary=pathlib.Path(temporary)/'test'
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O2','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',
  str(here/'pioneer_q38_recurrent_prefix_plan_mirror_test.cpp'),'-o',str(binary)],check=True,env=clean)
 subprocess.run([str(binary)],check=True,env={**clean,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
