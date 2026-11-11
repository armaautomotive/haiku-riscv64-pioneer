#!/usr/bin/env python3
"""Check tag142's isolated role grammar; this cannot build or open a driver."""
import os
from pathlib import Path
import subprocess

assert __debug__ and not os.environ.get('PYTHONOPTIMIZE')
assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
header=Path(__file__).resolve().parent.parent/'PioneerQ38QgwdTwoTokenRole.h'
compiler=os.environ.get('CXX','c++')
native=['-DPIONEER_Q38_QGWD_TWO_NATIVE_142=1','-DPIONEER_Q38_QGWD_NATIVE_141=1',
        '-D_KERNEL_MODE=1','-DPIONEER_DRIVER_BUILD_TAG=0x2026111100010142ULL']
client=['-DPIONEER_Q38_QGWD_TWO_CLIENT_142=1','-DPIONEER_Q38_QGWD_CLIENT_141=1']
def check(flags,okay,extra=''):
    source='#include "'+str(header)+'"\n'+extra
    result=subprocess.run([compiler,'-std=c++17',*flags,'-x','c++','-fsyntax-only','-'],
                          input=source,text=True,capture_output=True)
    assert (result.returncode==0)==okay,result.stderr[:1500]
check([],True);check(native,True);check(client,True)
for flags in (native,client):
    for selected in flags[1:]:
        check([f for f in flags if f!=selected],False)
    for forbidden in ('PIONEER_Q38_QGWD_MULTI_TOKEN_SIMULATION','PIONEER_Q38_WORK_INFERENCE',
                      'TT_PIONEER_Q38_WORK_MODE','PIONEER_Q38_CPU_VALIDATION',
                      'PIONEER_Q38_FULL_MODEL_CPU_REFERENCE','__FAST_MATH__'):
        check(flags+['-D'+forbidden+'=1'],False)
    selected=flags[0].split('=')[0]
    check([f for f in flags if f!=flags[0]]+[selected+'=0'],False)
    check(flags,False,'#undef '+selected[2:]+'\n#include "'+str(header)+'"\n')
check(native[:-1]+['-DPIONEER_DRIVER_BUILD_TAG=0x2026111000010141ULL'],False)
check(client+['-D_KERNEL_MODE=1'],False)
check(client+['-DPIONEER_DRIVER_BUILD_TAG=0x2026111100010142ULL'],False)
check(native+client,False)
check([],False,'#define PIONEER_Q38_QGWD_TWO_CLIENT_142 1\n#define PIONEER_Q38_QGWD_CLIENT_141 1\n#include "'+str(header)+'"\n')
print('PASS tag142 two-token role identity and exclusions; NO DRIVER OR DEVICE')
