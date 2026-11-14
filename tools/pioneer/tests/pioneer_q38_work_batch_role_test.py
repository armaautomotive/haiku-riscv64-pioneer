#!/usr/bin/env python3
"""Refuse mismatched Work-batch driver identities; no device access."""
import os
from pathlib import Path
import subprocess

root=Path(__file__).resolve().parents[3]
header=root/'src/add-ons/kernel/drivers/misc/PioneerDriverReloadProtocol.h'
compiler=os.environ.get('CXX','c++')
flags=['-D_KERNEL_MODE=1','-DTT_PIONEER_DRIVER_RELOAD=1','-DB_DEVICE_OP_CODES_END=1000',
       '-DPIONEER_DRIVER_BUILD_TAG=0x2026111300010152ULL',
       '-DPIONEER_Q38_WORK_BATCH_NATIVE_152=1','-DTT_PIONEER_EXPERIMENTAL_Q38_DISPATCH_VECTOR=1',
       '-DTT_PIONEER_Q38_WORK_MODE=1','-DPIONEER_Q38_COMPACT_ACTIVATIONS=1',
       '-DPIONEER_Q38_REUSE_ACTIVATIONS=1','-DPIONEER_Q38_REUSE_CROSS_QUANT=1']
def check(args,okay):
    result=subprocess.run([compiler,'-std=c++17','-x','c++','-fsyntax-only',*args,str(header)],capture_output=True,text=True)
    assert (result.returncode==0)==okay,result.stderr[:1500]
    if not okay:assert 'Work batch' in result.stderr or 'Tag152' in result.stderr,result.stderr[:1500]
check(flags,True)
for flag in ('-D_KERNEL_MODE=1','-DTT_PIONEER_DRIVER_RELOAD=1','-DPIONEER_Q38_WORK_BATCH_NATIVE_152=1',
             '-DTT_PIONEER_EXPERIMENTAL_Q38_DISPATCH_VECTOR=1','-DTT_PIONEER_Q38_WORK_MODE=1',
             '-DPIONEER_Q38_COMPACT_ACTIVATIONS=1','-DPIONEER_Q38_REUSE_ACTIVATIONS=1',
             '-DPIONEER_Q38_REUSE_CROSS_QUANT=1'):
    check([f for f in flags if f!=flag],False)
for flag in ('-DPIONEER_Q38_CPU_VALIDATION=1','-DPIONEER_Q38_FULL_MODEL_CPU_REFERENCE=1',
             '-DPIONEER_Q38_WORK_OUTPUT_DMA=1','-DPIONEER_Q38_NATIVE_Q8_LAYOUT=1',
             '-DPIONEER_Q38_FFN_CODE_DIAGNOSTIC=1','-DPIONEER_Q38_MODEL_ROOT_PROFILE=1',
             '-DPIONEER_Q38_ROOT_STATS_BENCH=1'):
    check(flags+[flag],False)
check([f for f in flags if not f.startswith('-DPIONEER_DRIVER_BUILD_TAG=')]+
      ['-DPIONEER_DRIVER_BUILD_TAG=0x2026102100010012ULL'],False)
print('PASS exact tag152 batch role and missing/wrong identity/CPU-mode refusals; NO DEVICE')
