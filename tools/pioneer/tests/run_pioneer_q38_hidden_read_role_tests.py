#!/usr/bin/env python3
"""Private QT3NODRV hidden readout role only;no native/hardware execution."""
import os
import pathlib
import re
import subprocess

assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
kernel = pathlib.Path(__file__).resolve().parents[3]/'src/add-ons/kernel/drivers/misc'
header = kernel/'PioneerQ38TokenHiddenReadRole.h'
native = (kernel/'PioneerQ38TokenNativeRole.h').read_text()
parents = set(re.findall(r'!defined\(([A-Z_][A-Z0-9_]*)\)\s*\|\|\s*\1\s*!=\s*1\b', native))
assert len(parents) == 21
flag = 'PIONEER_Q38_TOKEN_HELD_HIDDEN_DIAGNOSTIC'
selected = parents | {flag, 'PIONEER_Q38_TOKEN_PUSHED_NATIVE_CONTEXT', 'PIONEER_Q38_TOKEN_PUSHED_PROGRAMS'}
abi = ('api_version', 'init_hardware', 'init_driver', 'uninit_driver', 'publish_devices', 'find_device')
base = {**dict.fromkeys(selected, '1'), 'PIONEER_DRIVER_BUILD_TAG':'0x5154334e4f445256ULL',
        **{name:'pioneer_hidden_private_'+name for name in abi}}
foreign = set(re.findall(r'defined\(([A-Z_][A-Z0-9_]*)\)',
                        native[native.index('#if defined(PIONEER_Q38_EMBEDDING_NATIVE)'):]))
foreign |= {'PIONEER_Q38_TOKEN_VALIDATION_NATIVE', 'PIONEER_Q38_TOKEN_VALIDATION_CLIENT'}
assert not selected & foreign
accepted = refused = 0
include = '#include "'+str(header)+'"\n'
def defs(values):
    return ''.join(f'#define {name} {value}\n' for name,value in sorted(values.items()))
def check(values, okay, tail=''):
    global accepted, refused
    result = subprocess.run([os.environ.get('CXX','c++'), '-std=c++17','-Werror','-fsyntax-only','-x','c++','-'],
                            input=defs(values)+include+tail,text=True,stdout=subprocess.DEVNULL,stderr=subprocess.PIPE)
    assert (result.returncode == 0) == okay, (values,tail,result.stderr)
    if okay: accepted += 1
    else:
        assert 'error:' in result.stderr
        refused += 1
check(base, True, include)
check({}, True, include)
for name in sorted(selected):
    for value in (None,'0','2'):
        bad=base.copy()
        if value is None: del bad[name]
        else: bad[name]=value
        # An absent selector emits no capability,so test its late enable too.
        if name == flag and value is None:
            check(bad, False, '#define '+flag+' 1\n'+include)
        else: check(bad, False)
for name in abi:
    bad=base.copy();del bad[name];check(bad,False)
for value in ('0','0x51544b4e4f445256ULL','0x5154324e4f445256ULL','0x2026103000010074ULL'):
    bad=base.copy();bad['PIONEER_DRIVER_BUILD_TAG']=value;check(bad,False)
for name in sorted(foreign):
    for value in ('0','1','2'):
        bad=base.copy();bad[name]=value;check(bad,False)
for name in sorted(selected):
    check(base,False,'#undef '+name+'\n'+include)
    check(base,False,'#undef '+name+'\n#define '+name+' 0\n'+include)
    check(base,False,'#undef '+name+'\n#define '+name+' 2\n'+include)
print(f'PASS private QT3NODRV hidden readout role/{accepted} positive/{refused} missing+zero+two+late/tag/ABI/CPU/Work/old-physical refusals;NO opcode,native,physical,Work or TPS grant')
