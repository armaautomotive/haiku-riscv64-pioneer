#!/usr/bin/env python3
"""Production-only public sampler/readout boundaries;no driver/device execution."""
import os
import pathlib
import re
import runpy
import subprocess
import sys

assert sys.argv[1:]==[] and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
here=pathlib.Path(__file__).resolve().parent;saved=sys.argv[:]
try:
    sys.argv=[str(here/'run_pioneer_q38_token_constant_q4_native_roles.py'),'--public']
    old=runpy.run_path(sys.argv[0],run_name='__main__')
finally:sys.argv=saved
flags={'PIONEER_Q38_TOKEN_MATRIX_PHASE_PROGRAMS','PIONEER_Q38_TOKEN_MATRIX_PHASE_BENCHMARK_NATIVE',
       'PIONEER_Q38_TOKEN_MATRIX_PHASE_READ_NATIVE'}
base={**old['base'],**dict.fromkeys(flags,'1')};required={k for k,v in base.items() if v=='1'}
role=here.parent/'PioneerQ38TokenMatrixPhaseProgramsRole.h';read=here.parent/'PioneerQ38TokenMatrixPhaseReadRole.h'
boundary=here.parent/'PioneerQ38TokenMatrixPhaseDataBoundary.h';selected=here.parent/'PioneerQ38TokenSelectedPrograms.h'
include=lambda p:'#include "'+str(p)+'"\n';positive=negative=0
def check(values,okay,body=None):
    global positive,negative
    source=''.join('#define '+k+' '+v+'\n' for k,v in sorted(values.items()))
    source+=body if body is not None else include(role)+include(read)+include(role)
    r=subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Werror','-fsyntax-only','-x','c++','-'],
                     input=source,text=True,capture_output=True)
    assert (r.returncode==0)==okay,(values,body,r.stderr)
    if okay:positive+=1
    else:
        assert 'error:' in r.stderr;negative+=1
check(base,True)
check(base,True,include(selected)+include(selected)+'static_assert(PioneerQ38TokenMatrixPhaseDataSelection==3 && PioneerQ38TokenMatrixPhaseReadSelection==2 && PioneerQ38TokenPrograms::MatrixPhase==1);\n')
check(old['base'],True,include(selected)+'static_assert(PioneerQ38TokenMatrixPhaseDataSelection==0 && PioneerQ38TokenMatrixPhaseReadSelection==0 && PioneerQ38TokenPrograms::ConstantQ4Issue==1);\n')
for key in sorted(required):
    for value in (None,'0','2'):
        bad=base.copy()
        if value is None:del bad[key]
        else:bad[key]=value
        check(bad,False)
    for suffix in ('','#define '+key+' 0\n'):
        check(base,False,include(role)+'#undef '+key+'\n'+suffix+include(role))
foreign=(old['foreign']|set(re.findall(r'defined\(([A-Z_][A-Z0-9_]*)\)',boundary.read_text()+read.read_text())))-required
foreign-={'PIONEER_Q38_TOKEN_MATRIX_PHASE_PROGRAMS_H','PIONEER_Q38_TOKEN_MATRIX_PHASE_DATA_SELECTION_H'}
for key in sorted(foreign):
    for value in ('0','1'):
        check({**base,key:value},False)
        check(base,False,include(role)+'#define '+key+' '+value+'\n'+include(role))
for name in ('PioneerQ38TokenConstantQ4Programs.h','PioneerQ38TokenPhaseProfilePrograms.h','PioneerQ38TokenPrograms.h'):
    check(base,False,include(here.parent/name)+include(selected));check(base,False,include(selected)+include(here.parent/name))
for flag in flags:
    removed=base.copy();del removed[flag]
    check(removed,False,include(selected)+'#define '+flag+' 1\n'+include(selected))
print(f'PASS public sampler+fresh held reader/{positive} positives/{negative} production-only missing,zero,two,late,private,USER,shader,old-array,CPU+Work refusals/whole unchanged constant-Q4 PUBLIC+ancestors FIRST;NO native lifecycle,model,physical timing or deployment grant',flush=True)
