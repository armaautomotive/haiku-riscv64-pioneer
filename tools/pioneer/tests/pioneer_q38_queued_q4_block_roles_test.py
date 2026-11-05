#!/usr/bin/env python3
"""Offboard image boundary only; NO complete-worker/native/device grant."""
import os,pathlib,re,subprocess
if not __debug__ or os.environ.get('PYTHONOPTIMIZE'):raise SystemExit('Checks required')
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
here=pathlib.Path(__file__).resolve().parent
role=here.parent/'PioneerQ38QueuedQ4BlockRole.h'
header=here.parent/'PioneerQ38QueuedQ4Block.h'
required=set(re.findall(r'!defined\(([A-Z_][A-Z0-9_]*)\)\s*\|\|\s*\1\s*!=\s*1\b',role.read_text()))
assert len(required)==4;base={key:'1' for key in required}
include=lambda path:'#include "'+str(path)+'"\n'
positive=negative=0
def check(values,okay,body):
 global positive,negative
 source=''.join('#define '+k+' '+v+'\n' for k,v in sorted(values.items()))+body
 result=subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Werror','-fsyntax-only','-x','c++','-'],input=source,text=True,capture_output=True)
 assert (result.returncode==0)==okay,(values,body,result.stderr)
 if okay:positive+=1
 else:
  assert 'Queued Q4 block IMAGE requires' in result.stderr;negative+=1
check(base,True,include(header)+include(header))
for key in sorted(required):
 for value in (None,'0','2'):
  bad=base.copy()
  if value is None:del bad[key]
  else:bad[key]=value
  check(bad,False,include(header))
 for value in (None,'0','2'):
  body=include(header)+'#undef '+key+'\n'+('' if value is None else '#define '+key+' '+value+'\n')+include(header)
  check(base,False,body)
for key in ('_KERNEL_MODE','PIONEER_DRIVER_BUILD_TAG','TT_PIONEER_DRIVER_RELOAD','PIONEER_Q38_CPU_VALIDATION',
 'PIONEER_Q38_FULL_MODEL_CPU_REFERENCE','PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE',
 'PIONEER_Q38_FULL_OFFLOAD_NATIVE','PIONEER_Q38_FULL_OFFLOAD_CLIENT','__FAST_MATH__'):
 for value in ('0','1'):check({**base,key:value},False,include(header))
print(f'PASS queued-Q4 image-only component role:{positive} positive/{negative} missing,zero,two,late,native,CPU/Work/fast-math refusals;NO hardware qualification',flush=True)
