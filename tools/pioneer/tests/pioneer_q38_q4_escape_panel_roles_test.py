#!/usr/bin/env python3
"""Host DATA scan is not a native/Work/reference capability."""
import os,pathlib,subprocess
if not __debug__ or os.environ.get('PYTHONOPTIMIZE'):raise SystemExit('Checks required')
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
header=pathlib.Path(__file__).resolve().parent.parent/'PioneerQ38Q4EscapePanelScan.h'
include='#include "'+str(header)+'"\n'
positive=negative=0
def check(source,okay):
 global positive,negative
 r=subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Werror','-fsyntax-only','-x','c++','-'],input=source,text=True,capture_output=True)
 assert (r.returncode==0)==okay,(source,r.stderr)
 if okay:positive+=1
 else:
  assert 'Escape-panel scan is host DATA inventory only' in r.stderr;negative+=1
check(include+include,True)
for flag in ('_KERNEL_MODE','PIONEER_ATTENTION_FREESTANDING','PIONEER_Q38_WORK_INFERENCE','PIONEER_Q38_CPU_VALIDATION',
 'PIONEER_Q38_FULL_OFFLOAD_NATIVE','PIONEER_Q38_FULL_OFFLOAD_CLIENT','__FAST_MATH__'):
 for value in ('0','1'):
  define='#define '+flag+' '+value+'\n'
  check(define+include,False);check(include+define+include,False)
print(f'PASS escape-panel host DATA roles:{positive} positive/{negative} zero,one,late,kernel,native,client,Work,CPU-reference,fast-math refusals;NO hardware or speed permission',flush=True)
