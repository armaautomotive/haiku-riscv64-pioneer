#!/usr/bin/env python3
"""Original TEST byte/output boundary, not native execution qualification."""
import os
import pathlib
import re
import subprocess
here=pathlib.Path(__file__).resolve().parent
entry=here/'pioneer_q38_recurrent_prefix_original_native.h'
source=here.parent/'PioneerQ38RecurrentPrefixOriginalSource.h'
raw=entry.read_text();feature='PIONEER_Q38_RECURRENT_PREFIX_ORIGINAL_NATIVE_TEST'
forbidden=set(re.findall(r'defined\(([A-Z_][A-Z0-9_]*)\)',raw.split('#if defined('+feature,1)[0]))
assert len(forbidden)==8
def check(text,accept):
 result=subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Werror','-E','-x','c++','-'],input=text,
  text=True,stdout=subprocess.DEVNULL,stderr=subprocess.PIPE)
 assert (result.returncode==0)==accept,(accept,result.stderr)
include='#include "'+str(entry)+'"\n';check(include,True);check('#define '+feature+' 1\n'+include,True)
refused=0
for name in sorted(forbidden):
 for value in ('0','1'):
  define='#define '+name+' '+value+'\n'
  check(define+include,False);check(include+define+include,False);refused+=2
for value in ('0','2'):
 define='#define '+feature+' '+value+'\n'
 check(define+include,False);check(include+define+include,False);refused+=2
source_include='#include "'+str(source)+'"\n'
for value in ('0','1'):
 define='#define '+feature+' '+value+'\n'
 check(define+source_include,False);check(source_include+define+source_include,False);refused+=2
print('PASS original prefix native TEST boundary/8 kernel+CPU+image+Work aliases/optional exact1/CPU-oracle isolation/defined0+late refusals'+str(refused)+';NO native or hardware permission')
