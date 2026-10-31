#!/usr/bin/env python3
"""Physical fault-ROM revision perimeter;compile-only,NO accelerator access."""
import os
import pathlib
import re
import subprocess

assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
here=pathlib.Path(__file__).resolve().parent
kernel=here.parents[2]/'src/add-ons/kernel/drivers/misc'
parent=kernel/'PioneerQ38TokenHiddenValidationRole.h'
role=here.parent/'PioneerQ38TokenFaultProgramsRole.h'
physical=kernel/'PioneerQ38TokenFaultValidationRole.h'
pattern=r'!defined\(([A-Z_][A-Z0-9_]*)\)\s*\|\|\s*\1\s*!=\s*1\b'
required=set(re.findall(pattern,parent.read_text()))|{'PIONEER_Q38_TOKEN_FAULT_PROGRAMS','PIONEER_Q38_TOKEN_FAULT_VALIDATION_NATIVE'}
assert len(required)==25
text=parent.read_text();aliases=text[text.index('#if defined(PIONEER_Q38_TOKEN_VALIDATION_NATIVE)'):text.index('#if defined(api_version)')]
forbidden=set(re.findall(r'defined\(([A-Z_][A-Z0-9_]*)\)',aliases))
abi={'api_version','init_hardware','init_driver','uninit_driver','publish_devices','find_device'}
base={**dict.fromkeys(required,'1'),'PIONEER_DRIVER_BUILD_TAG':'0x2026103100010075ULL'}
accepted=refused=0
def include(path):return '#include "'+str(path)+'"\n'
def check(values,okay,header=role,tail=''):
    global accepted,refused
    source=''.join('#define '+k+' '+v+'\n' for k,v in sorted(values.items()))+include(header)+tail
    r=subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Werror','-fsyntax-only','-x','c++','-'],
                     input=source,text=True,stdout=subprocess.DEVNULL,stderr=subprocess.PIPE)
    assert (r.returncode==0)==okay,(values,tail,r.stderr)
    if okay:accepted+=1
    else:
        assert 'error:' in r.stderr,r.stderr
        refused+=1
for header in (role,physical):
    check(base,True,header,include(header))
    for name in sorted(required):
        for value in (None,'0','2'):
            bad=base.copy()
            if value is None:del bad[name]
            else:bad[name]=value
            check(bad,False,header)
        check(base,False,header,'#undef '+name+'\n#define '+name+' 0\n'+include(header))
    for name in sorted(forbidden|abi):
        for value in ('0','1'):check({**base,name:value},False,header)
        check(base,False,header,'#define '+name+' 0\n'+include(header))
    for tag in (None,'0','0x5154334e4f445256ULL','0x2026103000010074ULL','0x2026103100010076ULL'):
        bad=base.copy()
        if tag is None:del bad['PIONEER_DRIVER_BUILD_TAG']
        else:bad['PIONEER_DRIVER_BUILD_TAG']=tag
        check(bad,False,header)
old={k:v for k,v in base.items() if k not in ('PIONEER_Q38_TOKEN_FAULT_PROGRAMS','PIONEER_Q38_TOKEN_FAULT_VALIDATION_NATIVE')}
check(old,True,tail='static_assert(PioneerQ38TokenFaultProgramSelection==0);\n')
change='#define PIONEER_Q38_TOKEN_FAULT_PROGRAMS 1\n#define PIONEER_Q38_TOKEN_FAULT_VALIDATION_NATIVE 1\n'
check(old,False,tail=change+include(role))
change='#undef PIONEER_Q38_TOKEN_FAULT_VALIDATION_NATIVE\n#undef PIONEER_Q38_TOKEN_HIDDEN_VALIDATION_NATIVE\n'
change+='#define PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY 1\n#define PIONEER_Q38_TOKEN_HELD_HIDDEN_DIAGNOSTIC 1\n'
change+='#undef PIONEER_DRIVER_BUILD_TAG\n#define PIONEER_DRIVER_BUILD_TAG 0x5154334e4f445256ULL\n'
check(base,False,tail=change+include(role))
print(f'PASS separate explicit physical fault-ROM tag75 revision/{accepted} positives/{refused} missing+zero+two+late/ABI+oldtag+NODRV+CPU+Work+TEST refusals;numeric tag NOT byte identity;NO install/runtime/Work/TPS grant')
