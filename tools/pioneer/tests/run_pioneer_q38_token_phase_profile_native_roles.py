#!/usr/bin/env python3
"""Own full-token phase private/public native BUILD boundaries; no execution."""
import os
import pathlib
import re
import runpy
import subprocess
import sys

assert sys.argv[1:] in ([],['--public']) and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
public='--public' in sys.argv;here=pathlib.Path(__file__).resolve().parent;saved=sys.argv[:]
try:
    sys.argv=[str(here/'run_pioneer_q38_wide_gdn_native_roles.py'),*(['--public'] if public else [])]
    old=runpy.run_path(sys.argv[0],run_name='__main__')
finally:sys.argv=saved
primary='PIONEER_Q38_TOKEN_PHASE_PROFILE_PROGRAMS'
context='PIONEER_Q38_TOKEN_PHASE_PROFILE_'+('BENCHMARK_NATIVE' if public else 'NATIVE_CONTEXT')
base={**old['base'],primary:'1',context:'1'};required={k for k,v in base.items() if v=='1'}
role=here.parent/'PioneerQ38TokenPhaseProfileProgramsRole.h';boundary=here.parent/'PioneerQ38TokenPhaseProfileDataBoundary.h'
selected=here.parent/'PioneerQ38TokenSelectedPrograms.h';guard='PIONEER_Q38_TOKEN_PHASE_PROFILE_PROGRAMS_H'
positive=negative=0
include=lambda p:'#include "'+str(p)+'"\n'
def check(values,okay,body=None):
    global positive,negative
    source=''.join('#define '+key+' '+value+'\n' for key,value in sorted(values.items()))
    source+=body if body is not None else include(role)+include(role)
    result=subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Werror','-fsyntax-only','-x','c++','-'],input=source,text=True,capture_output=True)
    assert (result.returncode==0)==okay,(public,values,body,result.stderr)
    if okay:positive+=1
    else:
        assert 'error:' in result.stderr;negative+=1
check(base,True)
check(base,True,include(selected)+include(selected)+'static_assert(PioneerQ38TokenPhaseProfileDataSelection=='+str(3 if public else 2)+
      ' && PioneerQ38TokenPrograms::TokenPhaseProfile==1 && PioneerQ38TokenPrograms::WideGdn==1 && sizeof(PioneerQ38TokenPrograms::PeerGdn)==35944);\n')
check(old['base'],True,include(selected)+'static_assert(PioneerQ38TokenPhaseProfileDataSelection==0 && PioneerQ38TokenPrograms::WideGdn==1);\n')
for key in sorted(required):
    for value in (None,'0','2'):
        bad=base.copy()
        if value is None:del bad[key]
        else:bad[key]=value
        check(bad,False)
    check(base,False,include(role)+'#undef '+key+'\n#define '+key+' 0\n'+include(role))
    check(base,False,include(role)+'#undef '+key+'\n'+include(role))
foreign=old['foreign']|set(re.findall(r'defined\(([A-Z_][A-Z0-9_]*)\)',boundary.read_text()))
foreign-=required|{guard,'PIONEER_Q38_TOKEN_PHASE_PROFILE_DATA_SELECTION_H'}
for key in sorted(foreign):
    for value in ('0','1'):
        check({**base,key:value},False)
        check(base,False,include(role)+'#define '+key+' '+value+'\n'+include(role))
for flag in (primary,context):
    missing=base.copy();del missing[flag]
    check(missing,False,include(selected)+'#define '+flag+' 1\n'+include(selected))
    check(base,False,include(selected)+'#undef '+flag+'\n'+include(selected))
for name in ('PioneerQ38TokenWideGdnPrograms.h','PioneerQ38TokenBatchedOutputClosurePrograms.h','PioneerQ38TokenTwinBatchPrefetchPrograms.h',
             'PioneerQ38TokenBatchedPrefetchPrograms.h','PioneerQ38TokenPrefetchPrograms.h','PioneerQ38TokenFlatMatrixPrograms.h'):
    check(base,False,include(here.parent/name)+include(selected));check(base,False,include(selected)+include(here.parent/name))
print(f'PASS token-phase {"PUBLIC" if public else "PRIVATE"}/{positive} positives/{negative} missing,zero,two,late,opposite-native,old-array,HOST,USER,CPU+Work refusals;'
      'ALL unchanged returning-GDN+ancestors FIRST;NO native lifecycle,installation,model,hardware or speed grant',flush=True)
