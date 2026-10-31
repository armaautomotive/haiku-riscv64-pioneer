#!/usr/bin/env python3
"""Private reader/DATA role boundary only; no native lifecycle or device IO."""
import os
import pathlib
import re
import subprocess

assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
here=pathlib.Path(__file__).resolve().parent;kernel=here.parents[2]/'src/add-ons/kernel/drivers/misc'
native=(kernel/'PioneerQ38TokenNativeRole.h').read_text()
frontier=(kernel/'PioneerQ38TokenFrontierNativeRole.h').read_text()
role=here.parent/'PioneerQ38TokenFrontierProgramsRole.h'
pattern=r'!defined\(([A-Z_][A-Z0-9_]*)\)\s*\|\|\s*\1\s*!=\s*1\b'
required=set(re.findall(pattern,native))|set(re.findall(pattern,frontier))|{
    'PIONEER_Q38_TOKEN_PRECISE_BETA_PROGRAMS','PIONEER_Q38_TOKEN_FAULT_PROGRAMS','PIONEER_Q38_TOKEN_PUSHED_PROGRAMS'}
assert len(set(re.findall(pattern,native)))==21
abi=('api_version','init_hardware','init_driver','uninit_driver','publish_devices','find_device')
base={**dict.fromkeys(required,'1'),'PIONEER_DRIVER_BUILD_TAG':'0x5154334e4f445256ULL',
      **{name:'frontier_private_test_'+name for name in abi}}
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
    else:
        assert 'error:' in result.stderr;negative+=1
check(base,True)
selected=here.parent/'PioneerQ38TokenSelectedPrograms.h'
check(base,True,include(selected)+include(selected)+'static_assert(PioneerQ38TokenFrontierProgramSelection==2 && PioneerQ38TokenPrograms::TokenFrontier==1);\n')
old={key:value for key,value in base.items() if key not in ('PIONEER_Q38_TOKEN_FRONTIER_PROGRAMS','PIONEER_Q38_TOKEN_FRONTIER_NATIVE_CONTEXT')}
check(old,True,include(selected)+'static_assert(PioneerQ38TokenFrontierProgramSelection==0 && PioneerQ38TokenPrograms::JobBootFold==1);\n')
for key in sorted(required):
    for value in (None,'0','2'):
        bad=base.copy()
        if value is None:del bad[key]
        else:bad[key]=value
        check(bad,False)
    check(base,False,include(role)+'#undef '+key+'\n#define '+key+' 0\n'+include(role))
for key in abi:
    bad=base.copy();del bad[key];check(bad,False)
for value in (None,'0','0x51544b4e4f445256ULL','0x2026103100010075ULL'):
    bad=base.copy()
    if value is None:del bad['PIONEER_DRIVER_BUILD_TAG']
    else:bad['PIONEER_DRIVER_BUILD_TAG']=value
    check(bad,False)
foreign=(set(re.findall(r'defined\(([A-Z_][A-Z0-9_]*)\)',native+frontier+role.read_text()))
         -required-{'PIONEER_DRIVER_BUILD_TAG','PIONEER_Q38_TOKEN_FOOTPRINT_H'})
for key in sorted(foreign):
    for value in ('0','1'):check({**base,key:value},False)
    check(base,False,include(role)+'#define '+key+' 0\n'+include(role))
check(old,False,include(selected)+'#define PIONEER_Q38_TOKEN_FRONTIER_PROGRAMS 1\n#define PIONEER_Q38_TOKEN_FRONTIER_NATIVE_CONTEXT 1\n'+include(selected))
check(base,False,include(selected)+'#undef PIONEER_Q38_TOKEN_FRONTIER_NATIVE_CONTEXT\n'+include(selected))
print(f'PASS private frontier roles/{positive} syntax positives/{negative} missing+zero+two+late+tag+ABI+physical+CPU+Work refusals/old private selector still old firmware;syntax ONLY,NO runtime/public driver/model/physical/timing/Work/TPS qualification')
