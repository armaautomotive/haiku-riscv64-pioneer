#!/usr/bin/env python3
"""Compile-only isolation for the new head-band identity; no GPU or weights."""
import os
from pathlib import Path
import subprocess

root=Path(__file__).resolve().parents[3]
header=root/'src/add-ons/kernel/drivers/misc/PioneerNativeDramModelBandVariant.h'
compiler=os.environ.get('CXX','c++')
code='#include "'+str(header)+'"\n'
common=['-std=c++17','-Wall','-Wextra','-Werror','-x','c++','-fsyntax-only','-']
def check(flags, accepted, expected=''):
    result=subprocess.run([compiler,*common,*flags],input=code+expected,text=True,capture_output=True)
    if (result.returncode==0)!=accepted:
        raise AssertionError((flags,result.stderr[:2000]))
head=['-DPIONEER_NATIVE_DRAM_MODEL_BAND_ALIGNMENT64=1','-DPIONEER_QGW8_HEAD_BAND_130=1']
check(head,True,'static_assert(PioneerNativeDramModelBandVariant::Role==130);\n'
      'static_assert(PioneerNativeDramModelBandVariant::Tag==0x2026110800010130ULL);\n'
      'static_assert(PioneerNativeDramModelBandVariant::Opcode==0x9500);\n'
      'static_assert(PioneerNativeDramLoadVariant::L1Words*4==221488);\n')
check(['-DPIONEER_NATIVE_DRAM_MODEL_BAND_ALIGNMENT64=1'],True,
      'static_assert(PioneerNativeDramModelBandVariant::Role==84);\n'
      'static_assert(PioneerNativeDramModelBandVariant::Tag==0x2026110400010084ULL);\n')
check([],True,'static_assert(PioneerNativeDramModelBandVariant::Role==81);\n')
for bad in ('PIONEER_NATIVE_DRAM_LOAD_ALIGNMENT64','PIONEER_NATIVE_DRAM_LOAD_DIAGNOSTIC',
            'PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE',
            'PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_FULL_MODEL_CPU_REFERENCE'):
    check([*head,'-D'+bad+'=1'],False)
check(['-DPIONEER_QGW8_HEAD_BAND_130=1'],False)
check(['-DPIONEER_NATIVE_DRAM_MODEL_BAND_ALIGNMENT64=1','-DPIONEER_QGW8_HEAD_BAND_130=0'],False)
kernel=[*head,'-D_KERNEL_MODE=1','-DPIONEER_NATIVE_DRAM_MODEL_BAND_DIAGNOSTIC=1',
        '-DTT_PIONEER_DRIVER_RELOAD=1']
check([*kernel,'-DPIONEER_DRIVER_BUILD_TAG=0x2026110800010130ULL'],True)
check([*kernel,'-DPIONEER_DRIVER_BUILD_TAG=0x2026110400010084ULL'],False)
check([*head,'-D_KERNEL_MODE=1','-DPIONEER_DRIVER_BUILD_TAG=0x2026110800010130ULL'],False)
check(['-DPIONEER_NATIVE_DRAM_MODEL_BAND_ALIGNMENT64=1'],False,
      '#define PIONEER_QGW8_HEAD_BAND_130 1\n#include "'+str(header)+'"\n')
check(head,False,'#undef PIONEER_QGW8_HEAD_BAND_130\n#include "'+str(header)+'"\n')
print('PASS isolated head-band identity, old variants and 12 role refusals; no driver or device')
