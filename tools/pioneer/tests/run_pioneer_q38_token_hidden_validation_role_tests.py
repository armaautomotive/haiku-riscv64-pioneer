#!/usr/bin/env python3
"""Separate tag75 physical diagnostic role,compile-only. No device access."""
import os
import pathlib
import re
import subprocess

assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
kernel=pathlib.Path(__file__).resolve().parents[3]/'src/add-ons/kernel/drivers/misc'
native=kernel/'PioneerQ38TokenNativeRole.h'
physical=kernel/'PioneerQ38TokenHiddenValidationRole.h'
hidden=kernel/'PioneerQ38TokenHiddenReadRole.h'
oldphysical=kernel/'PioneerQ38TokenValidationRole.h'
pattern=r'!defined\(([A-Z_][A-Z0-9_]*)\)\s*\|\|\s*\1\s*!=\s*1\b'
parents=set(re.findall(pattern,native.read_text()))
required=set(re.findall(pattern,physical.read_text()))
flag='PIONEER_Q38_TOKEN_HIDDEN_VALIDATION_NATIVE'
oldflag='PIONEER_Q38_TOKEN_VALIDATION_NATIVE'
assert len(parents)==21 and required==(parents-{'PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY'})|{
    flag,'PIONEER_Q38_TOKEN_PUSHED_NATIVE_CONTEXT','PIONEER_Q38_TOKEN_PUSHED_PROGRAMS'}
role=physical.read_text()
aliases=role[role.index('#if defined('+oldflag+')'):role.index('#if defined(api_version)')]
forbidden=set(re.findall(r'defined\(([A-Z_][A-Z0-9_]*)\)',aliases))
assert not required & forbidden
oldrole=oldphysical.read_text()
oldaliases=oldrole[oldrole.index('#if defined(PIONEER_Q38_TOKEN_VALIDATION_CLIENT)'):oldrole.index('#if defined(api_version)')]
oldforbidden=set(re.findall(r'defined\(([A-Z_][A-Z0-9_]*)\)',oldaliases))
assert forbidden==(oldforbidden-{flag})|{oldflag}
abi=('api_version','init_hardware','init_driver','uninit_driver','publish_devices','find_device')
base={**dict.fromkeys(required,'1'),'PIONEER_DRIVER_BUILD_TAG':'0x2026103100010075ULL'}
accepted=refused=0
def defs(values):
    return ''.join(f'#define {name} {value}\n' for name,value in sorted(values.items()))
def inc(path):
    return '#include "'+str(path)+'"\n'
def check(values,okay,header=native,tail=''):
    global accepted,refused
    result=subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Werror','-fsyntax-only','-x','c++','-'],
        input=defs(values)+inc(header)+tail,text=True,stdout=subprocess.DEVNULL,stderr=subprocess.PIPE)
    assert (result.returncode==0)==okay,(values,tail,result.stderr)
    if okay:accepted+=1
    else:
        assert 'error:' in result.stderr
        refused+=1
for header in (native,physical,hidden):
    check(base,True,header,inc(header))
    for name in sorted(required):
        for value in (None,'0','2'):
            bad=base.copy()
            if value is None:del bad[name]
            else:bad[name]=value
            # An absent feature intentionally selects no hidden capability;
            # late activation must nevertheless refuse instead of promotion.
            tail=('#define '+flag+' 1\n'+inc(header)) if header==hidden and name==flag and value is None else ''
            check(bad,False,header,tail)
        check(base,False,header,'#undef '+name+'\n#define '+name+' 0\n'+inc(header))
    for name in sorted(forbidden|set(abi)):
        for value in ('0','1'):check({**base,name:value},False,header)
        check(base,False,header,'#define '+name+' 0\n'+inc(header))
    for tag in (None,'0','0x5154334e4f445256ULL','0x2026103000010074ULL','0x2026103000010075ULL'):
        bad=base.copy()
        if tag is None:del bad['PIONEER_DRIVER_BUILD_TAG']
        else:bad['PIONEER_DRIVER_BUILD_TAG']=tag
        check(bad,False,header)
old={**base};del old[flag];old[oldflag]='1';old['PIONEER_DRIVER_BUILD_TAG']='0x2026103000010074ULL'
check(old,True)
to_new='#undef '+oldflag+'\n#define '+flag+' 1\n#undef PIONEER_DRIVER_BUILD_TAG\n#define PIONEER_DRIVER_BUILD_TAG 0x2026103100010075ULL\n'
check(old,False,tail=to_new+inc(native))
to_old='#undef '+flag+'\n#define '+oldflag+' 1\n#undef PIONEER_DRIVER_BUILD_TAG\n#define PIONEER_DRIVER_BUILD_TAG 0x2026103000010074ULL\n'
check(base,False,tail=to_old+inc(native))
private={**dict.fromkeys(parents|{'PIONEER_Q38_TOKEN_PUSHED_NATIVE_CONTEXT','PIONEER_Q38_TOKEN_PUSHED_PROGRAMS'},'1'),
         'PIONEER_DRIVER_BUILD_TAG':'0x5154334e4f445256ULL',**{name:'private_'+name for name in abi}}
check(private,True)
to_new='#undef PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY\n'+''.join('#undef '+name+'\n' for name in abi)+to_new
check(private,False,tail=to_new+inc(native))
print(f'PASS separate tag75 diagnostic role/{accepted} positive/{refused} missing+zero+two+late/old74+NODRV+ABI+TEST+CPU+Work refusals;compile-only,NO runtime/install/Work/TPS grant')
