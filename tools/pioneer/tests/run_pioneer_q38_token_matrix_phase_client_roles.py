#!/usr/bin/env python3
"""Sampler USER containment;whole unchanged constant-Q4 client roles FIRST."""
import os
import pathlib
import re
import runpy
import subprocess
import sys

assert sys.argv[1:]==[] and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
here=pathlib.Path(__file__).resolve().parent
old=runpy.run_path(str(here/'run_pioneer_q38_token_constant_q4_client_roles.py'))
primary='PIONEER_Q38_TOKEN_MATRIX_PHASE_CLIENT';base={**old['base'],primary:'1'}
role=here.parent/'PioneerQ38TokenMatrixPhaseClientRole.h';cli=here.parent/'PioneerQ38TokenMatrixPhaseCli.h'
include=lambda p:'#include "'+str(p)+'"\n';positive=negative=0
def check(values,okay,body=None,selection=1):
    global positive,negative
    source=''.join('#define '+k+' '+v+'\n' for k,v in sorted(values.items()))
    source+=body if body is not None else include(role)+include(role)+f'static_assert(PioneerQ38TokenMatrixPhaseClientSelection=={selection});\n'
    r=subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Wall','-Wextra','-Werror','-fsyntax-only','-x','c++','-'],
                     input=source,text=True,capture_output=True)
    assert (r.returncode==0)==okay,(values,body,r.stderr)
    if okay:positive+=1
    else:
        assert 'error:' in r.stderr;negative+=1
check({},True,selection=0);check(old['base'],True,selection=0);check(base,True)
for key in sorted(base):
    for value in (None,'0','2'):
        bad=base.copy()
        if value is None:del bad[key]
        else:bad[key]=value
        check(bad,False)
    check(base,False,include(role)+'#undef '+key+'\n#define '+key+' 0\n'+include(role))
    check(base,False,include(role)+'#undef '+key+'\n'+include(role))
foreign=(old['foreign']|set(re.findall(r'defined\(([A-Z_][A-Z0-9_]*)\)',role.read_text())))-set(base)
for key in sorted(foreign):
    for value in ('0','1'):
        check({**base,key:value},False)
        check(base,False,include(role)+'#define '+key+' '+value+'\n'+include(role))
check(old['base'],False,include(role)+'#define '+primary+' 1\n'+include(role))
check(base,True,include(cli));check(old['base'],False,include(cli))
print(f'PASS sampler USER/{positive} positives/{negative} missing,zero,two,late,DATA,native,shader,CPU+Work refusals/whole unchanged constant-Q4+ancestors FIRST;NO native owner,physical timing,quality,Work or deployment grant',flush=True)
