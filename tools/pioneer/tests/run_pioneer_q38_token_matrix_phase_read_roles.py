#!/usr/bin/env python3
"""Separate fresh-slot reader PRIVATE boundaries;NO device access."""
import os
import pathlib
import re
import runpy
import subprocess
import sys

assert sys.argv[1:]==[] and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
here=pathlib.Path(__file__).resolve().parent;saved=sys.argv[:]
try:
    sys.argv=[str(here/'run_pioneer_q38_token_matrix_phase_native_roles.py')]
    old=runpy.run_path(sys.argv[0],run_name='__main__')
finally:sys.argv=saved
primary='PIONEER_Q38_TOKEN_MATRIX_PHASE_READ_NATIVE';base={**old['base'],primary:'1'}
role=here.parent/'PioneerQ38TokenMatrixPhaseReadRole.h';selected=here.parent/'PioneerQ38TokenSelectedPrograms.h'
required={k for k,v in base.items() if v=='1'};positive=negative=0
include=lambda p:'#include "'+str(p)+'"\n'
def check(values,okay,body=None):
    global positive,negative
    source=''.join('#define '+k+' '+v+'\n' for k,v in sorted(values.items()))
    source+=body if body is not None else include(role)+include(here.parent/'PioneerQ38TokenMatrixPhaseProgramsRole.h')+include(role)
    result=subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Werror','-fsyntax-only','-x','c++','-'],input=source,text=True,capture_output=True)
    assert (result.returncode==0)==okay,(values,body,result.stderr)
    if okay:positive+=1
    else:
        assert 'error:' in result.stderr;negative+=1
check(base,True)
check(base,True,include(selected)+include(selected)+'static_assert(PioneerQ38TokenMatrixPhaseReadSelection==1 && PioneerQ38TokenPrograms::MatrixPhase==1);\n')
check(old['base'],True,include(selected)+'static_assert(PioneerQ38TokenMatrixPhaseReadSelection==0);\n')
for key in sorted(required):
    for value in (None,'0','2'):
        bad=base.copy()
        if value is None:del bad[key]
        else:bad[key]=value
        if key==primary and value is None:
            # Omitting this OPTIONAL feature legitimately retains the old
            # sampler,whose SDK object has NO reader entry or result type.
            # Test that withdrawal as selection0,not a fake refusal.
            check(bad,True,include(here.parent/'PioneerQ38TokenMatrixPhaseProgramsRole.h')
                  +'static_assert(PioneerQ38TokenMatrixPhaseReadSelection==0);\n')
        else:check(bad,False)
    check(base,False,include(role)+'#undef '+key+'\n#define '+key+' 0\n'+include(here.parent/'PioneerQ38TokenMatrixPhaseProgramsRole.h'))
foreign=(old['foreign']|set(re.findall(r'defined\(([A-Z_][A-Z0-9_]*)\)',role.read_text())))-required
for key in sorted(foreign):
    for value in ('0','1'):
        check({**base,key:value},False)
        check(base,False,include(role)+'#define '+key+' '+value+'\n'+include(role))
check(old['base'],False,include(role)+'#define '+primary+' 1\n'+include(role))
check(base,False,include(role)+'#undef '+primary+'\n'+include(role))
print(f'PASS fresh-slot reader PRIVATE/{positive} positives/{negative} production-only missing,zero,two,late,parent,shader,USER,CPU+Work refusals/ALL sampler+older suites FIRST;NO native lifecycle,physical or performance grant',flush=True)
