#!/usr/bin/env python3
"""Compile-only tag/role isolation; no model, driver installation or device."""
import os
from pathlib import Path
import subprocess
import tempfile
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
here=Path(__file__).resolve().parent;root=here.parents[2]
compiler=os.environ.get('CXX','c++')
base=[compiler,'-std=c++17','-Wall','-Wextra','-Werror','-I'+str(here/'fixtures/qgwd-dense-source')]
protocol=root/'src/add-ons/kernel/drivers/misc/PioneerNativeModelBandProtocol.h'
lease=here.parent/'PioneerNativeModelBandLease.h'
def check(header,flags,accept,tail=''):
    source='#define B_DEVICE_OP_CODES_END 10000\n#include "'+str(header)+'"\n'+tail
    result=subprocess.run([*base,*flags,'-fsyntax-only','-x','c++','-'],input=source,text=True,capture_output=True)
    assert (result.returncode==0)==accept,(flags,result.stderr)
new='-DPIONEER_NATIVE_QGWD_DENSE_BAND_DIAGNOSTIC=1'
native=['-D_KERNEL_MODE=1','-DTT_PIONEER_DRIVER_RELOAD=1','-DPIONEER_NATIVE_MODEL_BAND_DIAGNOSTIC=1']
oldtag='-DPIONEER_DRIVER_BUILD_TAG=0x2026110400010080ULL'
newtag='-DPIONEER_DRIVER_BUILD_TAG=0x2026111000010140ULL'
check(protocol,[],True,'static_assert(PioneerNativeModelBandTag==0x2026110400010080ULL);\n')
check(protocol,[new],True,'static_assert(PioneerNativeModelBandTag==0x2026111000010140ULL);\n')
check(lease,[*native,oldtag],True)
check(lease,[*native,new,newtag],True)
for flags in ([*native,new,oldtag],[*native,newtag],[*native,'-DPIONEER_NATIVE_QGWD_DENSE_BAND_DIAGNOSTIC=0',newtag],
              [*native,new,newtag,'-DTT_PIONEER_Q38_WORK_MODE=1'],
              [*native,new,newtag,'-DPIONEER_Q38_FULL_MODEL_CPU_REFERENCE=1'],
              [*native,new,newtag,'-D__FAST_MATH__=1']):
    check(lease,flags,False)
for forbidden in ('TT_PIONEER_Q38_WORK_MODE','PIONEER_Q38_WORK_INFERENCE','PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_FULL_MODEL_CPU_REFERENCE'):
    check(protocol,[new,'-D'+forbidden+'=1'],False)
print('PASS distinct dense-band diagnostic tag/old-tag isolation and forbidden-role checks; no driver/device')
