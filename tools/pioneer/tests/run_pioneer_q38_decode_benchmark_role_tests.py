#!/usr/bin/env python3
"""Separate physical and USER compile roles only; no device IO or runtime grant."""
import os
import pathlib
import re
import subprocess
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
here=pathlib.Path(__file__).resolve().parent;kernel=here.parents[2]/'src/add-ons/kernel/drivers/misc'
physical=kernel/'PioneerQ38TokenDecodeBenchmarkRole.h'
client=here.parent/'PioneerQ38TokenDecodeBenchmarkClientRole.h'
parent=kernel/'PioneerQ38TokenHiddenValidationRole.h'
pattern=r'!defined\(([A-Z_][A-Z0-9_]*)\)\s*\|\|\s*\1\s*!=\s*1\b'
required=set(re.findall(pattern,parent.read_text()))|{
    'PIONEER_Q38_TOKEN_FAULT_PROGRAMS','PIONEER_Q38_TOKEN_FAULT_VALIDATION_NATIVE',
    'PIONEER_Q38_TOKEN_PRECISE_BETA_PROGRAMS','PIONEER_Q38_TOKEN_PRECISE_BETA_VALIDATION_NATIVE',
    'PIONEER_Q38_TOKEN_POSTED_CANCEL','PIONEER_Q38_TOKEN_JOB_BOOT_FOLD_PROGRAMS',
    'PIONEER_Q38_TOKEN_JOB_BOOT_FOLD_VALIDATION_NATIVE','PIONEER_Q38_TOKEN_DECODE_BENCHMARK_NATIVE'}
assert len(required)==31
text=parent.read_text();aliases=text[text.index('#if defined(PIONEER_Q38_TOKEN_VALIDATION_NATIVE)'):text.index('#if defined(api_version)')]
forbidden=set(re.findall(r'defined\(([A-Z_][A-Z0-9_]*)\)',aliases))|{
    'PIONEER_Q38_TOKEN_JOB_BOOT_FOLD_NATIVE_CONTEXT','PIONEER_Q38_TOKEN_DECODE_BENCHMARK_CLIENT'}
abi={'api_version','init_hardware','init_driver','uninit_driver','publish_devices','find_device'}
accepted=refused=0
def inc(path):return '#include "'+str(path)+'"\n'
def check(values,header,okay,tail=''):
    global accepted,refused
    source=''.join('#define '+key+' '+value+'\n' for key,value in sorted(values.items()))+inc(header)+tail
    result=subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Werror','-fsyntax-only','-x','c++','-'],
                          input=source,text=True,stdout=subprocess.DEVNULL,stderr=subprocess.PIPE)
    assert (result.returncode==0)==okay,(values,tail,result.stderr)
    if okay:accepted+=1
    else:
        assert 'error:' in result.stderr,result.stderr
        refused+=1
base={**dict.fromkeys(required,'1'),'PIONEER_DRIVER_BUILD_TAG':'0x2026103100010075ULL'}
check(base,physical,True,inc(physical))
for name in sorted(required):
    for value in (None,'0','2'):
        bad=base.copy()
        if value is None:del bad[name]
        else:bad[name]=value
        check(bad,physical,False)
    check(base,physical,False,'#undef '+name+'\n#define '+name+' 0\n'+inc(physical))
for name in sorted(forbidden|abi):
    for value in ('0','1'):check({**base,name:value},physical,False)
    check(base,physical,False,'#define '+name+' 0\n'+inc(physical))
for tag in (None,'0','0x5154334e4f445256ULL','0x2026103000010074ULL','0x2026103100010076ULL'):
    bad=base.copy()
    if tag is None:del bad['PIONEER_DRIVER_BUILD_TAG']
    else:bad['PIONEER_DRIVER_BUILD_TAG']=tag
    check(bad,physical,False)
user=dict.fromkeys(('PIONEER_Q38_TOKEN_DECODE_BENCHMARK_CLIENT','PIONEER_Q38_TOKEN_HIDDEN_VALIDATION_CLIENT'),'1')
check(user,client,True,inc(client))
for name in user:
    for value in (None,'0','2'):
        bad=user.copy()
        if value is None:del bad[name]
        else:bad[name]=value
        check(bad,client,False)
    check(user,client,False,'#undef '+name+'\n#define '+name+' 0\n'+inc(client))
user_forbidden=set(re.findall(r'defined\(([A-Z_][A-Z0-9_]*)\)',client.read_text()))-set(user)
for name in sorted(user_forbidden):
    for value in ('0','1'):check({**user,name:value},client,False)
    check(user,client,False,'#define '+name+' 0\n'+inc(client))
session=here.parent/'PioneerQ38TokenHiddenValidationSession.h'
packet={'B_DEVICE_OP_CODES_END':'1000'}
check({**packet,**user},session,True,inc(session))
short={**packet,'PIONEER_Q38_TOKEN_HIDDEN_VALIDATION_CLIENT':'1'}
check(short,session,True,inc(session))
check(short,session,False,'#define PIONEER_Q38_TOKEN_DECODE_BENCHMARK_CLIENT 1\n'+inc(session))
check({**packet,**user},session,False,'#undef PIONEER_Q38_TOKEN_DECODE_BENCHMARK_CLIENT\n'+inc(session))
for value in ('0','2'):check({**packet,**user,'PIONEER_Q38_TOKEN_DECODE_BENCHMARK_CLIENT':value},session,False)
for value in ('0','1'):check({**short,'PIONEER_Q38_TOKEN_DECODE_BENCHMARK_NATIVE':value},session,False)
print(f'PASS distinct34-position physical+USER roles/{accepted} positives/{refused} missing+zero+two+late/old ABI+NODRV+CPU+Work+TEST refusals;syntax ONLY,NO native/physical/numerical/Work/TPS grant')
