#!/usr/bin/env python3
"""Compile-only QGWF worker identity and incompatible-role checks."""
import os
from pathlib import Path
import subprocess

header=Path(__file__).resolve().parent.parent/'PioneerQ38DenseQ8MatrixWorkerLayout.h'
source='#include <cstdint>\n#include "'+str(header)+'"\n'
source+='static_assert(PioneerQ38DenseQ8MatrixWorkerLayout::Magic==0x46574751);\n'
source+='static_assert(PioneerQ38DenseQ8MatrixWorkerLayout::JobWitness==0x464f4751);\n'
names=('PIONEER_Q38_QGWF_BATCH4_WORKER','PIONEER_Q38_QGWE_BATCH_DENSE_WORKER',
       'PIONEER_Q38_QGW8_BFP8_BATCH_WORKER','PIONEER_Q38_BFP4_BATCH_INPUT_EXPERIMENT',
       'PIONEER_Q38_QGWD_SRCA_DENSE_Q8_WORKER','PIONEER_Q38_DENSE_Q8_MATRIX_WORKER_INSPECTION',
       'PIONEER_Q38_DENSE_BF16_SRCA_BASE_CANDIDATE','PIONEER_Q38_HEAD_MATRIX_WORKER_INSPECTION',
       'PIONEER_Q38_HYBRID_WORKER_IMAGE','PIONEER_Q38_BF16_WORKER_IMAGE',
       'PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION','PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT')
flags=['-D'+name+'=1' for name in names]
def check(args,okay,body=source):
    result=subprocess.run([os.environ.get('CXX','c++'),'-std=c++17',*args,'-fsyntax-only','-x','c++','-'],
                          input=body,text=True,capture_output=True)
    assert (result.returncode==0)==okay,result.stderr[:1000]
check(flags,True)
for name in names:
    reduced=[f for f in flags if f!='-D'+name+'=1']
    check(reduced,False);check(reduced+['-D'+name+'=0'],False)
for name in ('_KERNEL_MODE','PIONEER_DRIVER_BUILD_TAG','TT_PIONEER_DRIVER_RELOAD',
             'PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE','PIONEER_Q38_CPU_VALIDATION',
             'PIONEER_Q38_FULL_MODEL_CPU_REFERENCE','PIONEER_Q38_BFP4_INPUT_REUSE_EXPERIMENT',
             'PIONEER_Q38_BFP8_INPUT_REUSE_EXPERIMENT','PIONEER_Q38_BFP4_BULK_LOAD_EXPERIMENT'):
    check(flags+['-D'+name+'=1'],False)
for name in ('PIONEER_Q38_QGWF_BATCH4_WORKER','PIONEER_Q38_BFP4_BATCH_INPUT_EXPERIMENT'):
    check(flags,False,source+'#undef '+name+'\n#include "'+str(header)+'"\n')
print('PASS isolated QGWF identity and incompatible-role refusals; NO DEVICE OR SPEED')
