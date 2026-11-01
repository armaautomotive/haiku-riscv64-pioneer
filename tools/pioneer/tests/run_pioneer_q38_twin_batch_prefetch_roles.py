#!/usr/bin/env python3
"""Separate twin-batch shader syntax/refusals;NO execution/native authority."""
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
    sys.argv=[str(here/'run_pioneer_q38_batched_prefetch_roles.py')]
    old=runpy.run_path(sys.argv[0],run_name='__main__')
finally:
    sys.argv=saved
role=here.parent/'PioneerQ38TokenTwinBatchPrefetchRole.h'
primary='PIONEER_Q38_TOKEN_TWIN_BATCH_PREFETCH_WORKER_IMAGE'
base={**old['math'],primary:'1','PIONEER_Q38_WORK_TWIN_BATCH_PREFETCH':'1','PIONEER_Q38_TWIN_BATCH_PARENT_BINDING':'1'}
all_flags=set(re.findall(r'defined\(([A-Z_][A-Z0-9_]*)\)',role.read_text()))
positive=negative=0
includes=''.join('#include "'+str(path)+'"\n' for path in
                (role,here.parent/'PioneerQ38TokenBatchedPrefetchRole.h',here.parent/'PioneerQ38TokenPrefetchWorkerRole.h'))
def check(values,okay,body=None):
    global positive,negative
    code=''.join('#define '+key+' '+value+'\n' for key,value in sorted(values.items()))
    code+=body if body is not None else includes+includes+'static_assert(PioneerQ38TokenTwinBatchPrefetchSelection==1);\n'
    result=subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Wall','-Wextra','-Werror','-fsyntax-only','-x','c++','-'],
                          input=code,text=True,capture_output=True)
    assert (result.returncode==0)==okay,(values,body,result.stderr)
    if okay:positive+=1
    else:
        assert 'error:' in result.stderr
        negative+=1
check(base,True)
for key in sorted(base):
    for value in (None,'0','2'):
        wrong=base.copy()
        if value is None:del wrong[key]
        else:wrong[key]=value
        check(wrong,False)
    check(base,False,includes+'#undef '+key+'\n#define '+key+' 0\n'+includes)
for key in sorted(all_flags-set(base)):
    for value in ('0','1'):check({**base,key:value},False)
    check(base,False,includes+'#define '+key+' 0\n'+includes)
check({},False,'#include "'+str(role)+'"\n'+''.join('#define '+key+' 1\n' for key in sorted(base))+includes)
check(base,False,includes+'#undef '+primary+'\n'+includes)
for key in (primary,'PIONEER_Q38_WORK_TWIN_BATCH_PREFETCH','PIONEER_Q38_TWIN_BATCH_PARENT_BINDING'):
    for value in ('0','1'):check({key:value},False)
check(base,False,includes+'static_assert(PioneerQ38TokenTwinBatchPrefetchSelection==0,"old batched math leaf");\n')
check({**old['sender'],primary:'1','PIONEER_Q38_WORK_TWIN_BATCH_PREFETCH':'1','PIONEER_Q38_TWIN_BATCH_PARENT_BINDING':'1'},False)
check(old['math'],True,includes+'static_assert(PioneerQ38TokenTwinBatchPrefetchSelection==0);\n')
check({},True,includes+'static_assert(PioneerQ38TokenTwinBatchPrefetchSelection==0);\n')
print(f'PASS distinct twin-batch shader/{positive} positives/{negative} missing,zero,two,late,old-leaf,sender,native,ROOT,oldDATA,CPU+Work refusals;'
      'ALL old batched+prefetch roles FIRST;NO emitted execution,native ownership,physical or TPS proof')
