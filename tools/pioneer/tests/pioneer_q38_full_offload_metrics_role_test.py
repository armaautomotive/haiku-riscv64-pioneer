#!/usr/bin/env python3
"""Full-card counter native role boundaries;syntax only,NO driver/card run."""
import os
import pathlib
import re
import subprocess

assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
here=pathlib.Path(__file__).resolve().parent
kernel=here.parents[2]/'src/add-ons/kernel/drivers/misc'
role=kernel/'PioneerQ38FullOffloadRole.h'
required=set(re.findall(r'!defined\(([A-Z_][A-Z0-9_]*)\)\s*\|\|\s*\1\s*!=\s*1\b',role.read_text()))
assert len(required)==23
base={name:'1' for name in required}
base.update(PIONEER_DRIVER_BUILD_TAG='0x2026110400010090ULL',PIONEER_Q38_FULL_OFFLOAD_BATCHED='1',
            PIONEER_Q38_FULL_OFFLOAD_METRICS='1')
include=lambda p:'#include "'+str(p)+'"\n'
positive=negative=0
def check(values,okay,body=None):
    global positive,negative
    source=''.join('#define '+name+' '+value+'\n' for name,value in sorted(values.items()))
    source+=body if body is not None else include(role)+include(role)
    result=subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Wall','-Wextra','-Werror',
                           '-fsyntax-only','-x','c++','-'],input=source,text=True,capture_output=True)
    assert (result.returncode==0)==okay,(values,body,result.stderr)
    if okay:positive+=1
    else:
        assert 'error:' in result.stderr;negative+=1
check(base,True)
selected=here.parent/'PioneerQ38TokenSelectedPrograms.h'
check(base,True,include(selected)+include(selected)+
      'static_assert(PioneerQ38FullOffloadMetricsSelection==1 && PioneerQ38TokenPrograms::EogCount==5 && PioneerQ38TokenPrograms::TokenFrontier==1);\n')
for name in sorted(base):
    for value in (None,'0','2'):
        bad=base.copy()
        if value is None:del bad[name]
        else:bad[name]=value
        check(bad,False)
    check(base,False,include(role)+'#undef '+name+'\n'+include(role))
for tag in ('0x2026110400010088ULL','0x2026110400010089ULL','0x2026103100010075ULL'):
    check({**base,'PIONEER_DRIVER_BUILD_TAG':tag},False)
for name in ('PIONEER_Q38_TOKEN_FRONTIER_NATIVE_CONTEXT','PIONEER_Q38_TOKEN_FRONTIER_BENCHMARK_NATIVE',
             'PIONEER_Q38_TOKEN_FRONTIER_BENCHMARK_CLIENT','PIONEER_Q38_CPU_VALIDATION',
             'PIONEER_Q38_FULL_MODEL_CPU_REFERENCE','PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE',
             'PIONEER_Q38_FULL_OFFLOAD_CLIENT','api_version','__FAST_MATH__'):
    for value in ('0','1'):
        check({**base,name:value},False)
        check(base,False,include(role)+'#define '+name+' '+value+'\n'+include(role))
# Both directions must detect a late feature/tag change even though the
# firmware DATA itself deliberately stays byte-identical.
plain=base.copy();del plain['PIONEER_Q38_FULL_OFFLOAD_METRICS'];plain['PIONEER_DRIVER_BUILD_TAG']='0x2026110400010089ULL'
check(plain,True)
check(plain,False,include(role)+'#define PIONEER_Q38_FULL_OFFLOAD_METRICS 1\n#undef PIONEER_DRIVER_BUILD_TAG\n#define PIONEER_DRIVER_BUILD_TAG 0x2026110400010090ULL\n'+include(role))
check(base,False,include(role)+'#undef PIONEER_Q38_FULL_OFFLOAD_METRICS\n#undef PIONEER_DRIVER_BUILD_TAG\n#define PIONEER_DRIVER_BUILD_TAG 0x2026110400010089ULL\n'+include(role))
print(f'PASS full-card metrics role:{positive} positives/{negative} missing,zero,wrong,late,old-native/CPU/Work/client/ABI refusals;same immutable firmware NOT diagnostic authority;NO hardware')
