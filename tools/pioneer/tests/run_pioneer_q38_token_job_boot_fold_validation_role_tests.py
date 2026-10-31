#!/usr/bin/env python3
"""Distinct physical optimized-code perimeter;syntax ONLY,no accelerator IO."""
import os
import pathlib
import re
import subprocess
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
here=pathlib.Path(__file__).resolve().parent
kernel=here.parents[2]/'src/add-ons/kernel/drivers/misc'
parent=kernel/'PioneerQ38TokenHiddenValidationRole.h'
physical=kernel/'PioneerQ38TokenJobBootFoldValidationRole.h'
role=here.parent/'PioneerQ38TokenJobBootFoldProgramsRole.h'
pattern=r'!defined\(([A-Z_][A-Z0-9_]*)\)\s*\|\|\s*\1\s*!=\s*1\b'
required=set(re.findall(pattern,parent.read_text()))|{
    'PIONEER_Q38_TOKEN_FAULT_PROGRAMS','PIONEER_Q38_TOKEN_FAULT_VALIDATION_NATIVE',
    'PIONEER_Q38_TOKEN_PRECISE_BETA_PROGRAMS','PIONEER_Q38_TOKEN_PRECISE_BETA_VALIDATION_NATIVE',
    'PIONEER_Q38_TOKEN_POSTED_CANCEL','PIONEER_Q38_TOKEN_JOB_BOOT_FOLD_PROGRAMS',
    'PIONEER_Q38_TOKEN_JOB_BOOT_FOLD_VALIDATION_NATIVE'}
assert len(required)==30
text=parent.read_text();aliases=text[text.index('#if defined(PIONEER_Q38_TOKEN_VALIDATION_NATIVE)'):text.index('#if defined(api_version)')]
forbidden=set(re.findall(r'defined\(([A-Z_][A-Z0-9_]*)\)',aliases))|{'PIONEER_Q38_TOKEN_JOB_BOOT_FOLD_NATIVE_CONTEXT'}
abi={'api_version','init_hardware','init_driver','uninit_driver','publish_devices','find_device'}
base={**dict.fromkeys(required,'1'),'PIONEER_DRIVER_BUILD_TAG':'0x2026103100010075ULL'}
accepted=refused=0
def inc(path):return '#include "'+str(path)+'"\n'
def check(values,okay,header=role,tail=''):
    global accepted,refused
    source=''.join('#define '+k+' '+v+'\n' for k,v in sorted(values.items()))+inc(header)+tail
    result=subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Werror','-fsyntax-only','-x','c++','-'],
                          input=source,text=True,stdout=subprocess.DEVNULL,stderr=subprocess.PIPE)
    assert (result.returncode==0)==okay,(values,tail,result.stderr)
    if okay:accepted+=1
    else:
        assert 'error:' in result.stderr,result.stderr
        refused+=1
for header in (role,physical):
    check(base,True,header,inc(header))
    for name in sorted(required):
        for value in (None,'0','2'):
            bad=base.copy()
            if value is None:del bad[name]
            else:bad[name]=value
            check(bad,False,header)
        check(base,False,header,'#undef '+name+'\n#define '+name+' 0\n'+inc(header))
    for name in sorted(forbidden|abi):
        for value in ('0','1'):check({**base,name:value},False,header)
        check(base,False,header,'#define '+name+' 0\n'+inc(header))
    for tag in (None,'0','0x5154334e4f445256ULL','0x2026103000010074ULL','0x2026103100010076ULL'):
        bad=base.copy()
        if tag is None:del bad['PIONEER_DRIVER_BUILD_TAG']
        else:bad['PIONEER_DRIVER_BUILD_TAG']=tag
        check(bad,False,header)
old={k:v for k,v in base.items() if k not in ('PIONEER_Q38_TOKEN_JOB_BOOT_FOLD_PROGRAMS','PIONEER_Q38_TOKEN_JOB_BOOT_FOLD_VALIDATION_NATIVE')}
check(old,True,tail='static_assert(PioneerQ38TokenJobBootFoldSelection==0);\n')
new='#define PIONEER_Q38_TOKEN_JOB_BOOT_FOLD_PROGRAMS 1\n#define PIONEER_Q38_TOKEN_JOB_BOOT_FOLD_VALIDATION_NATIVE 1\n'
check(old,False,tail=new+inc(role))
check(base,False,tail='#undef PIONEER_Q38_TOKEN_JOB_BOOT_FOLD_VALIDATION_NATIVE\n'+inc(role))
data=dict.fromkeys(('PIONEER_Q38_TOKEN_JOB_BOOT_FOLD_PROGRAMS','PIONEER_Q38_TOKEN_PRECISE_BETA_PROGRAMS',
                    'PIONEER_Q38_TOKEN_FAULT_PROGRAMS','PIONEER_Q38_TOKEN_PUSHED_PROGRAMS'),'1')
check(data,True,tail='static_assert(PioneerQ38TokenJobBootFoldSelection==1);\n')
check(data,False,tail=new+inc(role))
print(f'PASS separate physical job-boot-fold revision/{accepted} positives/{refused} missing+zero+two+late/ABI+oldtag+NODRV+CPU+Work+TEST refusals;explicit new code selection3 required,tag NOT byte identity;NO install/runtime/Work/TPS grant')
