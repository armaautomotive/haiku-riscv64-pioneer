#!/usr/bin/env python3
"""Shader syntax/refusals only;NO native/runtime/physical authority."""
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
    sys.argv=[str(here/'run_pioneer_q38_token_prefetch_worker_roles.py')]
    old=runpy.run_path(sys.argv[0],run_name='__main__')
finally:
    sys.argv=saved
role=here.parent/'PioneerQ38TokenBatchedPrefetchRole.h'
primary='PIONEER_Q38_TOKEN_BATCHED_PREFETCH_WORKER_IMAGE'
math={**old['base'],primary:'1','PIONEER_Q38_WORK_BATCHED_PREFETCH':'1'}
sender={primary:'1','PIONEER_Q38_PRECISE_WORKER_IMAGE':'1','PIONEER_Q38_PUSH_COMPLETION_WORKER_IMAGE':'1'}
all_flags=set(re.findall(r'defined\(([A-Z_][A-Z0-9_]*)\)',role.read_text()))
positive=negative=0
def include(kind):
    parent=here.parent/('PioneerQ38TokenPrefetchWorkerRole.h' if kind==1 else 'PioneerQ38PreciseWorkerImageRole.h')
    return '#include "'+str(role)+'"\n#include "'+str(parent)+'"\n'
def check(values,kind,okay,body=None):
    global positive,negative
    code=''.join('#define '+key+' '+value+'\n' for key,value in sorted(values.items()))
    # Mirror the ACTUAL new leaf TU's required selection. The unselected
    # old sender remains legitimate,but cannot become this new candidate.
    code+=body if body is not None else include(kind)+include(kind)+f'static_assert(PioneerQ38TokenBatchedPrefetchSelection=={kind});\n'
    result=subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Wall','-Wextra','-Werror','-fsyntax-only','-x','c++','-'],
                          input=code,text=True,capture_output=True)
    assert (result.returncode==0)==okay,(values,body,result.stderr)
    if okay:positive+=1
    else:
        assert 'error:' in result.stderr
        negative+=1
for kind,base in ((1,math),(2,sender)):
    check(base,kind,True,include(kind)+include(kind)+f'static_assert(PioneerQ38TokenBatchedPrefetchSelection=={kind});\n')
    for key in sorted(base):
        for value in (None,'0','2'):
            wrong=base.copy()
            if value is None:del wrong[key]
            else:wrong[key]=value
            check(wrong,kind,False)
        check(base,kind,False,include(kind)+'#undef '+key+'\n#define '+key+' 0\n'+include(kind))
    for key in sorted(all_flags-set(base)):
        for value in ('0','1'):
            check({**base,key:value},kind,False)
        check(base,kind,False,include(kind)+'#define '+key+' 0\n'+include(kind))
    check({},kind,False,'#include "'+str(role)+'"\n'+''.join('#define '+key+' 1\n' for key in sorted(base))+include(kind))
    check(base,kind,False,include(kind)+'#undef '+primary+'\n'+include(kind))
check({**math,**sender},1,False)
check({'PIONEER_Q38_WORK_BATCHED_PREFETCH':'0'},1,False)
check({'PIONEER_Q38_WORK_BATCHED_PREFETCH':'1'},1,False)
check({},1,True,'#include "'+str(role)+'"\nstatic_assert(PioneerQ38TokenBatchedPrefetchSelection==0);\n')
print(f'PASS distinct batched shader/{positive} positives/{negative} missing,zero,two,late,math-vs-sender,native,ROOT,oldDATA,CPU+Work refusals;'
      'old prefetch role controls FIRST;NO emitted execution,native ownership,physical or TPS proof')
