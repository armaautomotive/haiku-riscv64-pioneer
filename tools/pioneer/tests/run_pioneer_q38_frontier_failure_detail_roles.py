#!/usr/bin/env python3
"""Private syntax perimeter only. No device,source,owner or execution proof."""
import os
import pathlib
import runpy
import subprocess
import sys

assert len(sys.argv)==1 and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
here=pathlib.Path(__file__).resolve().parent
saved=sys.argv[:]
try:
    sys.argv=[str(here/'run_pioneer_q38_twin_batch_prefetch_native_roles.py')]
    old=runpy.run_path(sys.argv[0],run_name='__main__')
finally:
    sys.argv=saved
flag='PIONEER_Q38_TOKEN_FRONTIER_FAILURE_DETAIL_NATIVE'
base={**old['base'],flag:'1'};required=old['required']|{flag}
role=here.parents[2]/'src/add-ons/kernel/drivers/misc/PioneerQ38TokenFrontierFailureDetailRole.h'
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
check(old['base'],True,include(role)+'static_assert(PioneerQ38TokenFrontierFailureDetailSelection==0);\n')
for key in sorted(required):
    for value in (None,'0','2'):
        bad=base.copy()
        if value is None:del bad[key]
        else:bad[key]=value
        # Removing ONLY the opt-in is the unchanged old profile,not a refusal.
        if key==flag and value is None:
            check(bad,False,include(role)+'static_assert(PioneerQ38TokenFrontierFailureDetailSelection==1);\n')
        else:check(bad,False)
    check(base,False,include(role)+'#undef '+key+'\n#define '+key+' 0\n'+include(role))
for key in old['old']['old']['abi']:
    bad=base.copy();del bad[key];check(bad,False)
    check(base,False,include(role)+'#undef '+key+'\n'+include(role))
for value in old['old']['old']['bad_tags']:
    bad=base.copy()
    if value is None:del bad['PIONEER_DRIVER_BUILD_TAG']
    else:bad['PIONEER_DRIVER_BUILD_TAG']=value
    check(bad,False)
for key in sorted(old['foreign']|{'PIONEER_Q38_TOKEN_FRONTIER_FAILURE_DETAIL_BENCHMARK_NATIVE'}):
    for value in ('0','1'):check({**base,key:value},False)
    check(base,False,include(role)+'#define '+key+' 0\n'+include(role))
check(old['base'],False,include(role)+'#define '+flag+' 1\n'+include(role))
check(base,False,include(role)+'#undef '+flag+'\n'+include(role))
print(f'PASS failure-detail PRIVATE/{positive} syntax positives/{negative} missing,zero,two,late,tag,ABI,public,USER,'
      'shader,CPU+Work refusals;ALL unchanged twin+ancestors FIRST;NO native lifecycle,install,hardware or TPS grant')
