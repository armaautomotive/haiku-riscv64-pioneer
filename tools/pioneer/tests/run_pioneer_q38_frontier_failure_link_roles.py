#!/usr/bin/env python3
"""QFL1 private/public/TEST syntax perimeters only; no execution grant."""
import os
import pathlib
import runpy
import subprocess
import sys

assert sys.argv[1:] in ([],['--public'],['--native-test'])
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
kind='public' if '--public' in sys.argv else 'TEST' if '--native-test' in sys.argv else 'PRIVATE'
here=pathlib.Path(__file__).resolve().parent
parent=here/('run_pioneer_q38_frontier_failure_detail_benchmark_roles.py' if kind=='public'
            else 'pioneer_q38_frontier_failure_detail_native_role_test.py' if kind=='TEST'
            else 'run_pioneer_q38_frontier_failure_detail_roles.py')
saved=sys.argv[:]
try:
    sys.argv=[str(parent)];old=runpy.run_path(str(parent),run_name='__main__')
finally:
    sys.argv=saved
sys.stdout.flush()
native='PIONEER_Q38_TOKEN_FRONTIER_FAILURE_LINK_NATIVE'
public='PIONEER_Q38_TOKEN_FRONTIER_FAILURE_LINK_BENCHMARK_NATIVE'
fixture='PIONEER_Q38_TOKEN_NATIVE_FRONTIER_FAILURE_LINK_FIXTURE'
flags={native}|({public} if kind=='public' else {fixture} if kind=='TEST' else set())
base={**old['base'],**dict.fromkeys(flags,'1')}
required={key for key,value in base.items() if value=='1'}
production=here.parents[2]/'src/add-ons/kernel/drivers/misc/PioneerQ38TokenFrontierFailureLinkRole.h'
role=here/'pioneer_q38_token_native_frontier_failure_link_role.h' if kind=='TEST' else production
positive=negative=0
def include(path):return '#include "'+str(path)+'"\n'
def check(values,okay,body=None):
    global positive,negative
    source=''.join('#define '+key+' '+value+'\n' for key,value in sorted(values.items()))
    source+=body if body is not None else include(role)+include(role)
    result=subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Werror','-fsyntax-only','-x','c++','-'],
                          input=source,text=True,capture_output=True)
    assert (result.returncode==0)==okay,(kind,values,body,result.stderr)
    if okay:positive+=1
    else:assert 'error:' in result.stderr;negative+=1
wanted=2 if kind=='public' else 1
check(base,True)
check(base,True,include(role)+'static_assert(PioneerQ38TokenFrontierFailureLinkSelection=='+str(wanted)+');\n')
check(old['base'],True,include(production)+'static_assert(PioneerQ38TokenFrontierFailureLinkSelection==0);\n')
for key in sorted(required):
    for value in (None,'0','2'):
        bad=base.copy()
        if value is None:del bad[key]
        else:bad[key]=value
        body=include(role)+'static_assert(PioneerQ38TokenFrontierFailureLinkSelection==1);\n' if kind=='PRIVATE' and key==native and value is None else None
        check(bad,False,body)
    check(base,False,include(role)+'#undef '+key+'\n#define '+key+' 0\n'+include(role))
for key in ('api_version','init_hardware','init_driver','uninit_driver','publish_devices','find_device'):
    if key in base:
        bad=base.copy();del bad[key];check(bad,False)
        check(base,False,include(role)+'#undef '+key+'\n'+include(role))
for tag in (None,'0','0x2026102100010012ULL','0x5154324e4f445256ULL'):
    bad=base.copy()
    if tag is None:del bad['PIONEER_DRIVER_BUILD_TAG']
    else:bad['PIONEER_DRIVER_BUILD_TAG']=tag
    check(bad,False)
foreign=old.get('foreign',old.get('old',{}).get('foreign',set()))|{
    'PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE','PIONEER_Q38_CPU_VALIDATION',
    'PIONEER_Q38_FULL_MODEL_CPU_REFERENCE','PIONEER_Q38_TOKEN_FRONTIER_FAILURE_REPORT_CLIENT',
    'PIONEER_Q38_NATIVE_ROOT_PROFILE_FIXTURE','PIONEER_Q38_TOKEN_NATIVE_FULL_FLOW_FIXTURE',
    'PIONEER_Q38_TOKEN_NATIVE_CONTROL_FIXTURE'}
foreign|={fixture} if kind=='public' else {public}
for key in sorted(foreign-required):
    for value in ('0','1'):check({**base,key:value},False)
    check(base,False,include(role)+'#define '+key+' 0\n'+include(role))
for flag in flags:
    missing=base.copy();del missing[flag]
    first=role if flag==fixture else production
    check(missing,False,include(first)+'#define '+flag+' 1\n'+include(role))
    check(base,False,include(role)+'#undef '+flag+'\n'+include(role))
print(f'PASS QFL1 {kind}/{positive} syntax positives/{negative} missing,zero,two,late,tag,ABI,foreign,USER,CPU+Work refusals;'
      'ALL unchanged QFD1+ancestors FIRST;NO initialization,model,native lifetime,installation,hardware cause or TPS qualification')
