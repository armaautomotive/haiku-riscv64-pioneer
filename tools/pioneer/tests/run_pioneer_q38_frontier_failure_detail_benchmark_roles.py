#!/usr/bin/env python3
"""Public diagnostic syntax ONLY. Never stage,install or execute a driver."""
import os
import pathlib
import runpy
import subprocess
import sys

assert len(sys.argv)==1 and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
here=pathlib.Path(__file__).resolve().parent
saved=sys.argv[:]
try:
    sys.argv=[str(here/'run_pioneer_q38_twin_batch_prefetch_benchmark_roles.py')]
    old=runpy.run_path(sys.argv[0],run_name='__main__')
finally:
    sys.argv=saved
flags={'PIONEER_Q38_TOKEN_FRONTIER_FAILURE_DETAIL_NATIVE','PIONEER_Q38_TOKEN_FRONTIER_FAILURE_DETAIL_BENCHMARK_NATIVE'}
base={**old['base'],**dict.fromkeys(flags,'1')};required=old['required']|flags
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
check(base,True,include(role)+'static_assert(PioneerQ38TokenFrontierFailureDetailSelection==2);\n')
check(old['base'],True,include(role)+'static_assert(PioneerQ38TokenFrontierFailureDetailSelection==0);\n')
for key in sorted(required):
    for value in (None,'0','2'):
        bad=base.copy()
        if value is None:del bad[key]
        else:bad[key]=value
        check(bad,False)
    check(base,False,include(role)+'#undef '+key+'\n#define '+key+' 0\n'+include(role))
for value in (None,'0','0x5154334e4f445256ULL','0x2026102100010012ULL','0x2026110200010076ULL'):
    bad=base.copy()
    if value is None:del bad['PIONEER_DRIVER_BUILD_TAG']
    else:bad['PIONEER_DRIVER_BUILD_TAG']=value
    check(bad,False)
foreign=old['foreign']|{'PIONEER_Q38_TOKEN_NATIVE_FRONTIER_FAILURE_DETAIL_FIXTURE'}
for key in sorted(foreign-flags):
    for value in ('0','1'):check({**base,key:value},False)
    check(base,False,include(role)+'#define '+key+' 0\n'+include(role))
for flag in flags:
    missing=base.copy();del missing[flag]
    check(missing,False,include(role)+'#define '+flag+' 1\n'+include(role))
    check(base,False,include(role)+'#undef '+flag+'\n'+include(role))
for name in ('PioneerQ38TokenPrograms.h','PioneerQ38TokenBatchedPrefetchPrograms.h',
             'PioneerQ38TokenPrefetchPrograms.h','PioneerQ38TokenFlatMatrixPrograms.h'):
    check(base,False,include(here.parent/name)+include(role))
    check(base,False,include(role)+include(here.parent/name))
print(f'PASS failure-detail public diagnostic/{positive} syntax positives/{negative} missing,zero,two,late,tag,ABI,'
      'old-array,private,USER,shader,CPU+Work refusals;ALL unchanged twin-public+ancestors FIRST;NO native lifecycle,'
      'hardware execution,installation,model or TPS qualification')
