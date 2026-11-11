#!/usr/bin/env python3
"""Compile-only QGWE controller identity, parent and isolation checks."""
import os
from pathlib import Path
import subprocess

root = Path(__file__).resolve().parent.parent
role = root/'PioneerQ38QgwdRootImageRole.h'
header = root/'PioneerQ38DenseQ8MatrixRootFrame.h'
include = '#include "'+str(role)+'"\n'
names = ('PIONEER_Q38_QGWE_BATCH_DENSE_ROOT','PIONEER_Q38_QGWD_ROOT_IMAGE',
         'PIONEER_Q38_HEAD_MATRIX_ROOT_INSPECTION','PIONEER_Q38_GLOBAL_CHAIN_HYBRID_BFP',
         'PIONEER_Q38_GLOBAL_CHAIN_BF16_FPU','PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION',
         'PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT','PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE')
flags = ['-D'+name+'=1' for name in names]
def check(args, okay, code=include, compiler=None):
    result=subprocess.run([compiler or os.environ.get('CXX','c++'),'-std=c++17',*args,'-fsyntax-only','-x','c++','-'],
                          input=code,text=True,capture_output=True)
    assert (result.returncode==0)==okay,result.stderr[:1200]
check(flags,True)
for name in names:
    reduced=[flag for flag in flags if flag!='-D'+name+'=1']
    check(reduced,name=='PIONEER_Q38_QGWE_BATCH_DENSE_ROOT')
    check(reduced+['-D'+name+'=0'],False)
for name in ('_KERNEL_MODE','PIONEER_DRIVER_BUILD_TAG','TT_PIONEER_DRIVER_RELOAD',
             'PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE','PIONEER_Q38_CPU_VALIDATION',
             'PIONEER_Q38_FULL_MODEL_CPU_REFERENCE','PIONEER_Q38_QGWE_BATCH_DENSE_WORKER',
             'PIONEER_Q38_QGW8_BFP8_BATCH_WORKER','PIONEER_Q38_QGWA_BATCH_ROOT_IMAGE'):
    check(flags+['-D'+name+'=1'],False)
check(flags,False,include+'#undef PIONEER_Q38_QGWE_BATCH_DENSE_ROOT\n'+include)
check(flags,False,include+'#undef PIONEER_Q38_QGWD_ROOT_IMAGE\n'+include)
source='#include "'+str(header)+'"\n'
source+='static_assert(PioneerQ38DenseQ8MatrixWorkerLayout::Magic==0x45574751);\n'
source+='static_assert(PioneerQ38DenseQ8MatrixWorkerLayout::JobWitness==0x454f4751);\n'
cross='/Volumes/HaikuBuildLocal/generated.riscv64/cross-tools-riscv64/bin/riscv64-unknown-haiku-g++'
check(flags+['-ffreestanding','-march=rv32imf','-mabi=ilp32',
             '-DPIONEER_Q38_DENSE_Q8_MATRIX_WORKER_INSPECTION=1',
             '-DPIONEER_Q38_DENSE_Q8_MATRIX_ROOT_INSPECTION=1',
             '-DPIONEER_ATTENTION_FREESTANDING=1','-DPIONEER_DRAM_SWEEP_FREESTANDING=1'],True,source,cross)
plan_header=root/'PioneerQ38HybridHeadWorkerLayout.h'
plan_source='#include <cstdint>\n#include "'+str(plan_header)+'"\n'
plan_source+='static_assert(PioneerQ38HybridHeadWorkerLayout::Magic==0x45574751);\n'
plan_flags=['-D'+name+'=1' for name in ('PIONEER_Q38_QGWE_BATCH_DENSE_PLAN',
            'PIONEER_Q38_QGWD_PLAN_SIMULATION','PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION',
            'PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION','PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT')]
check(plan_flags,True,plan_source)
for name in ('PIONEER_Q38_QGWD_PLAN_SIMULATION','PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION'):
    check([f for f in plan_flags if f!='-D'+name+'=1'],False,plan_source)
for name in ('_KERNEL_MODE','PIONEER_DRIVER_BUILD_TAG','PIONEER_Q38_WORK_INFERENCE',
             'PIONEER_Q38_QGWE_BATCH_DENSE_ROOT','PIONEER_Q38_QGWE_BATCH_DENSE_WORKER'):
    check(plan_flags+['-D'+name+'=1'],False,plan_source)
check(plan_flags,False,plan_source+'#undef PIONEER_Q38_QGWE_BATCH_DENSE_PLAN\n#include "'+str(plan_header)+'"\n')
print('PASS isolated QGWE root identity, parent and incompatible role refusals; NO DEVICE')
