#!/usr/bin/env python3
"""Compile-only QGWF ROOT/plan isolation and identity."""
import os
from pathlib import Path
import subprocess

root=Path(__file__).resolve().parent.parent;role=root/'PioneerQ38QgwdRootImageRole.h'
source='#include "'+str(role)+'"\n'
names=('PIONEER_Q38_QGWF_DUAL_BATCH_ROOT','PIONEER_Q38_QGWE_BATCH_DENSE_ROOT','PIONEER_Q38_QGWD_ROOT_IMAGE',
       'PIONEER_Q38_HEAD_MATRIX_ROOT_INSPECTION','PIONEER_Q38_GLOBAL_CHAIN_HYBRID_BFP','PIONEER_Q38_GLOBAL_CHAIN_BF16_FPU',
       'PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION','PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT','PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE')
flags=['-D'+name+'=1' for name in names]
def check(args,okay,body=source,compiler=None):
    result=subprocess.run([compiler or os.environ.get('CXX','c++'),'-std=c++17',*args,'-fsyntax-only','-x','c++','-'],
                          input=body,capture_output=True,text=True)
    assert (result.returncode==0)==okay,result.stderr[:1000]
check(flags,True)
for name in names:
    reduced=[f for f in flags if f!='-D'+name+'=1']
    check(reduced,name=='PIONEER_Q38_QGWF_DUAL_BATCH_ROOT')
    check(reduced+['-D'+name+'=0'],False)
for name in ('_KERNEL_MODE','PIONEER_DRIVER_BUILD_TAG','PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE',
             'PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_FULL_MODEL_CPU_REFERENCE','PIONEER_Q38_QGWF_BATCH4_WORKER',
             'PIONEER_Q38_QGWF_DUAL_BATCH_PLAN'):
    check(flags+['-D'+name+'=1'],False)
check(flags,False,source+'#undef PIONEER_Q38_QGWF_DUAL_BATCH_ROOT\n'+source)
header=root/'PioneerQ38DenseQ8MatrixRootFrame.h'
body='#include "'+str(header)+'"\nstatic_assert(PioneerQ38DenseQ8MatrixWorkerLayout::Magic==0x46574751);\n'
cross='/Volumes/HaikuBuildLocal/generated.riscv64/cross-tools-riscv64/bin/riscv64-unknown-haiku-g++'
check(flags+['-ffreestanding','-march=rv32imf','-mabi=ilp32','-DPIONEER_Q38_DENSE_Q8_MATRIX_WORKER_INSPECTION=1',
             '-DPIONEER_Q38_DENSE_Q8_MATRIX_ROOT_INSPECTION=1','-DPIONEER_ATTENTION_FREESTANDING=1',
             '-DPIONEER_DRAM_SWEEP_FREESTANDING=1'],True,body,cross)
print('PASS QGWF ROOT identity and isolated-role refusals; NO DEVICE')
