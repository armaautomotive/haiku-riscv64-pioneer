#!/usr/bin/env python3
"""Separate ROOT syntax/refusals only;NO emitted/native/device authority."""
import os
import pathlib
import re
import runpy
import subprocess
import sys

assert len(sys.argv)==1 and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
here=pathlib.Path(__file__).resolve().parent
saved=sys.argv[:]
try:
    sys.argv=[str(here/'run_pioneer_q38_flat_matrix_grammar_roles.py')]
    old=runpy.run_path(sys.argv[0],run_name='__main__')
finally:
    sys.argv=saved
role=here.parent/'PioneerQ38BatchedPrefetchRootRole.h';include='#include "'+str(role)+'"\n'
primary='PIONEER_Q38_BATCHED_PREFETCH_ROOT_IMAGE'
all_flags=set(re.findall(r'defined\(([A-Z_][A-Z0-9_]*)\)',role.read_text()))
positive=negative=0
def check(values,okay,tail='',selected=1):
    global positive,negative
    source=''.join('#define '+key+' '+value+'\n' for key,value in sorted(values.items()))+include+include
    source+='static_assert(PioneerQ38BatchedPrefetchRootSelection=='+str(selected)+',"expected ROOT build");\n'+tail
    result=subprocess.run([os.environ.get('CXX','c++'),'-std=c++11','-Werror','-fsyntax-only','-x','c++','-'],input=source,text=True,capture_output=True)
    assert (result.returncode==0)==okay,(values,tail,result.stderr)
    if okay:positive+=1
    else:assert 'error:' in result.stderr;negative+=1
for target,other in (('PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE','PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE'),
                     ('PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE','PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE')):
    base={**old['base'],primary:'1',target:'1'};check(base,True)
    for key in base:
        for value in (None,'0','2'):
            bad=base.copy()
            if value is None:del bad[key]
            else:bad[key]=value
            check(bad,False)
        check(base,False,'#undef '+key+'\n#define '+key+' 0\n'+include)
    check(base,False,'#undef '+primary+'\n'+include)
    for key in sorted(all_flags-set(base)):
        for value in ('0','1'):check({**base,key:value},False)
        check(base,False,'#define '+key+' 0\n'+include)
    check(base,False,'#define '+other+' 1\n'+include)
check({},True,selected=0)
check({},False,'#define '+primary+' 1\n'+include,selected=0)
print(f'PASS batched ROOT/{positive} syntax positives/{negative} missing,zero,two,late,wrong-architecture,shader,oldDATA,native,CPU+Work refusals;'
      'ALL unchanged flat grammar controls FIRST;NO emitted arguments,connected model,native ownership,hardware or TPS proof')
