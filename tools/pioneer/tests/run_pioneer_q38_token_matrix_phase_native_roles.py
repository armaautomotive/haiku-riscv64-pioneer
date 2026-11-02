#!/usr/bin/env python3
"""Separate sampler PRIVATE BUILD boundaries;production checks,not test assertions."""
import os
import pathlib
import re
import runpy
import subprocess
import sys

assert sys.argv[1:]==[] and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
here=pathlib.Path(__file__).resolve().parent;saved=sys.argv[:]
try:
    sys.argv=[str(here/'run_pioneer_q38_token_constant_q4_native_roles.py')]
    old=runpy.run_path(sys.argv[0],run_name='__main__')
finally:sys.argv=saved
primary='PIONEER_Q38_TOKEN_MATRIX_PHASE_PROGRAMS';context='PIONEER_Q38_TOKEN_MATRIX_PHASE_NATIVE_CONTEXT'
base={**old['base'],primary:'1',context:'1'};required={k for k,v in base.items() if v=='1'}
role=here.parent/'PioneerQ38TokenMatrixPhaseProgramsRole.h';boundary=here.parent/'PioneerQ38TokenMatrixPhaseDataBoundary.h'
selected=here.parent/'PioneerQ38TokenSelectedPrograms.h';guard='PIONEER_Q38_TOKEN_MATRIX_PHASE_PROGRAMS_H'
positive=negative=0;include=lambda p:'#include "'+str(p)+'"\n'
def check(values,okay,body=None):
    global positive,negative
    source=''.join('#define '+key+' '+value+'\n' for key,value in sorted(values.items()))
    source+=body if body is not None else include(role)+include(role)
    result=subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Werror','-fsyntax-only','-x','c++','-'],input=source,text=True,capture_output=True)
    assert (result.returncode==0)==okay,(values,body,result.stderr)
    if okay:positive+=1
    else:
        assert 'error:' in result.stderr;negative+=1
check(base,True)
check(base,True,include(selected)+include(selected)+'static_assert(PioneerQ38TokenMatrixPhaseDataSelection==2 && PioneerQ38TokenPrograms::MatrixPhase==1 && PioneerQ38TokenPrograms::MatrixPhaseVersion==2 && PioneerQ38TokenPrograms::TokenPhaseProfile==1);\n')
check(old['base'],True,include(selected)+'static_assert(PioneerQ38TokenMatrixPhaseDataSelection==0 && PioneerQ38TokenPrograms::ConstantQ4Issue==1);\n')
for key in sorted(required):
    for value in (None,'0','2'):
        bad=base.copy()
        if value is None:del bad[key]
        else:bad[key]=value
        check(bad,False)
    check(base,False,include(role)+'#undef '+key+'\n#define '+key+' 0\n'+include(role))
    check(base,False,include(role)+'#undef '+key+'\n'+include(role))
foreign=old['foreign']|set(re.findall(r'defined\(([A-Z_][A-Z0-9_]*)\)',boundary.read_text()))
foreign-=required|{guard,'PIONEER_Q38_TOKEN_MATRIX_PHASE_DATA_SELECTION_H'}
for key in sorted(foreign):
    for value in ('0','1'):
        check({**base,key:value},False)
        check(base,False,include(role)+'#define '+key+' '+value+'\n'+include(role))
for flag in (primary,context):
    missing=base.copy();del missing[flag]
    check(missing,False,include(selected)+'#define '+flag+' 1\n'+include(selected))
    check(base,False,include(selected)+'#undef '+flag+'\n'+include(selected))
for name in ('PioneerQ38TokenConstantQ4Programs.h','PioneerQ38TokenPhaseProfilePrograms.h','PioneerQ38TokenWideGdnPrograms.h',
             'PioneerQ38TokenTwinBatchPrefetchPrograms.h','PioneerQ38TokenPrograms.h'):
    check(base,False,include(here.parent/name)+include(selected));check(base,False,include(selected)+include(here.parent/name))
print(f'PASS separate returning sampler PRIVATE/{positive} positives/{negative} production-only role refusals/'
      'ALL unchanged constant-Q4+ancestors FIRST/no public,USER,Work,old-array,CPU-oracle alias;'
      'NO native lifecycle,held metric readout,model,hardware or speed grant',flush=True)
