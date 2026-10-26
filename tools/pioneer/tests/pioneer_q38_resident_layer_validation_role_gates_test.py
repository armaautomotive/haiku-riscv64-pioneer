#!/usr/bin/env python3
"""Exact DEBUG69 role gates;preprocessing ONLY,NO driver/card execution."""
import os
import pathlib
import subprocess

here=pathlib.Path(__file__).resolve().parent
kernel=here.parents[2]/'src/add-ons/kernel/drivers/misc'
header='#include "'+str(kernel/'PioneerQ38ResidentLayerValidationRole.h')+'"\n'
import re
roles=''.join((kernel/name).read_text() for name in ('PioneerQ38ResidentLayerValidationRole.h','PioneerQ38ResidentLayerOwnerRole.h','PioneerQ38ResidentLayerNativeRole.h','PioneerQ38ResidentOutputOwnerRole.h'))
parents=set(re.findall(r'!defined\(([A-Z][A-Z0-9_]*)\)\s*\|\|\s*\1\s*!=\s*1\b',roles))
assert len(parents)==31
flags={k:'1' for k in parents};flags['PIONEER_DRIVER_BUILD_TAG']='0x2026102600010069ULL'
clean={k:v for k,v in os.environ.items() if not k.startswith('PIONEER_')}
count=0
def check(source,roles,expected):
    global count
    result=subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-E','-x','c++','-I'+str(kernel),
                           *['-D'+k+'='+v for k,v in roles.items()],'-'],input=source,text=True,capture_output=True,
                          env=clean,timeout=10)
    assert (result.returncode==0)==expected,(roles,result.stderr)
    count+=1
check(header,flags,True);check(header+header,flags,True)
for key in flags:
    missing=dict(flags);del missing[key];check(header,missing,False)
    check(header+'#undef '+key+'\n'+header,flags,False)
    for value in ('','0','2','-1'):
        check(header,{**flags,key:value},False)
        check(header+'#undef '+key+'\n#define '+key+' '+value+'\n'+header,flags,False)
for tag in ('0x51524c4e4f445256ULL','0x2026102500010068ULL','0x2026102500010067ULL','0x2026102500010065ULL','0x2026102400010052ULL','0','1'):
    check(header,{**flags,'PIONEER_DRIVER_BUILD_TAG':tag},False)
for key in sorted(set(re.findall(r'defined\(([A-Z_][A-Z0-9_]*)\)',roles))-set(flags)):
    for value in ('0','1'):
        check(header,{**flags,key:value},False)
        check(header+'#define '+key+' '+value+'\n'+header,flags,False)
for key in ('api_version','init_hardware','init_driver','uninit_driver','publish_devices','find_device'):
    for value in ('0','unexpected_alias'):
        check(header,{**flags,key:value},False)
native=(kernel/'tenstorrent_pci_probe.cpp').read_text()
prefix=native.split('#if defined(PIONEER_Q38_RESIDENT_OPERATOR_IMAGE)',1)[0].replace('#include "tenstorrent_pci_probe.h"','',1)
check(prefix,flags,True)
for value in ('0','1','unexpected_alias'):
    check(prefix,{**flags,'PIONEER_Q38_RESIDENT_LAYER_PHYSICAL_ROLE_TAG_OK':value},False)
for tag in ('0x2026102500010067ULL','0x2026102600010069ULL'):
    check(prefix,{'TT_PIONEER_DRIVER_RELOAD':'1','PIONEER_DRIVER_BUILD_TAG':tag},False)
for value in ('0','1'):
    check(prefix,{'PIONEER_Q38_RESIDENT_OUTPUT_OWNER':value},False)
print('PASS exact DEBUG69 role gates'+str(count)+'/complete parents/old native+DEBUG+Work+CPU+image+compile67+client aliases/late changes/ABI alias+internal-witness spoofing/bare reserved tags;preprocessing ONLY,NO native/card grant')
