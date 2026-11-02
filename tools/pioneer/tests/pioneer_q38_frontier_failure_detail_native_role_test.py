#!/usr/bin/env python3
"""Syntax-only native TEST perimeter;no initialization or execution grant."""
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
    sys.argv=[str(here/'run_pioneer_q38_frontier_failure_detail_roles.py')]
    old=runpy.run_path(sys.argv[0],run_name='__main__')
finally:
    sys.argv=saved
role=here/'pioneer_q38_token_native_frontier_failure_detail_role.h'
ancestor=here/'pioneer_q38_token_native_root_frontend_role.h'
pattern=r'!defined\(([A-Z_][A-Z0-9_]*)\)\s*\|\|\s*\1\s*!=\s*1\b'
required=set(re.findall(pattern,role.read_text()+ancestor.read_text()))
base={**old['base'],**dict.fromkeys(required,'1')}
positive=negative=0
def include(path):return '#include "'+str(path)+'"\n'
def check(values,okay,body=None):
    global positive,negative
    source=''.join('#define '+key+' '+value+'\n' for key,value in sorted(values.items()))
    source+=body if body is not None else include(role)+include(role)
    result=subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Werror','-fsyntax-only','-x','c++','-'],
                          input=source,text=True,capture_output=True)
    assert (result.returncode==0)==okay,(values,body,result.stderr)
    if okay:positive+=1
    else:assert 'error:' in result.stderr;negative+=1
check(base,True)
check(base,True,include(role)+'static_assert(PioneerQ38TokenFrontierFailureDetailSelection==1);\n')
for key in sorted(required):
    for value in (None,'0','2'):
        bad=base.copy()
        if value is None:del bad[key]
        else:bad[key]=value
        check(bad,False)
    check(base,False,include(role)+'#undef '+key+'\n#define '+key+' 0\n'+include(role))
for key in ('PIONEER_Q38_NATIVE_ROOT_PROFILE_FIXTURE','PIONEER_Q38_TOKEN_NATIVE_FULL_FLOW_FIXTURE',
            'PIONEER_Q38_TOKEN_NATIVE_CONTROL_FIXTURE','PIONEER_Q38_TOKEN_NATIVE_HOLD_RECHECK_FIXTURE',
            'PIONEER_Q38_TOKEN_VALIDATION_NATIVE','PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE',
            'PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_FULL_MODEL_CPU_REFERENCE',
            'PIONEER_Q38_TOKEN_TWIN_BATCH_PREFETCH_BENCHMARK_NATIVE',
            'PIONEER_Q38_TOKEN_FRONTIER_FAILURE_DETAIL_BENCHMARK_NATIVE',
            'PIONEER_Q38_TOKEN_FRONTIER_FAILURE_REPORT_CLIENT'):
    for value in ('0','1'):check({**base,key:value},False)
    check(base,False,include(role)+'#define '+key+' 0\n'+include(role))
print(f'PASS failure-detail native TEST/{positive} syntax positives/{negative} missing,zero,two,late,shortened,profile,'
      'public,USER,CPU+Work refusals;ALL unchanged production-private+ancestors FIRST;NOT instantiated native lifecycle,'
      'source/model execution,hardware or TPS qualification')
