#!/usr/bin/env python3
"""Compile-only selector checks for the isolated QGWA root image."""
import os
from pathlib import Path
import subprocess

if any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ):raise SystemExit('role override refused')
role=Path(__file__).resolve().parent.parent/'PioneerQ38QgwaBatchRootImageRole.h'
include='#include "'+str(role)+'"\n';compiler=os.environ.get('CXX','c++')
names=('PIONEER_Q38_QGWA_BATCH_ROOT_IMAGE','PIONEER_Q38_HEAD_MATRIX_ROOT_INSPECTION',
       'PIONEER_Q38_GLOBAL_CHAIN_HYBRID_BFP','PIONEER_Q38_GLOBAL_CHAIN_BF16_FPU',
       'PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION','PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT',
       'PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE')
flags=['-D'+name+'=1' for name in names]
def check(options,okay,source=include):
    result=subprocess.run([compiler,'-std=c++17',*options,'-fsyntax-only','-x','c++','-'],input=source,text=True,capture_output=True)
    assert (result.returncode==0)==okay,result.stderr[:1000]
check(flags,True)
component=flags[:-1]+['-DPIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE=1'];check(component,True)
for name in names:
    reduced=[f for f in flags if f!='-D'+name+'=1']
    check(reduced,name==names[0]);check(reduced+['-D'+name+'=0'],False)
for name in ('PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE','PIONEER_Q38_QGWA_BATCH_ROOT_RECEIPT_INSPECTION',
             'PIONEER_Q38_QGW8_BFP8_BATCH_WORKER','PIONEER_Q38_QGW8_BFP8_REUSE_WORKER',
             'PIONEER_Q38_QGW8_LAYER_TRACE_IMAGE','PIONEER_Q38_QGW8_DENSE_INPUT_TRACE_IMAGE',
             'PIONEER_Q38_QGW8_DENSE_STAGE_TRACE_IMAGE','PIONEER_Q38_QGW8_COARSE_PROFILE_IMAGE',
             '_KERNEL_MODE','PIONEER_DRIVER_BUILD_TAG','TT_PIONEER_DRIVER_RELOAD','PIONEER_Q38_TOKEN_NATIVE_CONTEXT',
             'PIONEER_Q38_HYBRID_FULL_NATIVE','PIONEER_Q38_FULL_OFFLOAD_NATIVE','PIONEER_Q38_HYBRID_FULL_CLIENT',
             'PIONEER_Q38_FULL_OFFLOAD_CLIENT','PIONEER_Q38_BF16_FULL_NATIVE','PIONEER_Q38_BF16_FULL_CLIENT',
             'PIONEER_Q38_QGW8_CANDIDATE_NATIVE_128','PIONEER_Q38_QGW8_CANDIDATE_CLIENT_128',
             'PIONEER_Q38_QGW8_TRACE_NATIVE_131','PIONEER_Q38_QGW8_TRACE_CLIENT_131',
             'PIONEER_Q38_QGWA_BATCH_PROGRAMS_INSPECTION','PIONEER_Q38_QGW8_PROGRAMS_INSPECTION','PIONEER_Q38_WORK_INFERENCE',
             'TT_PIONEER_Q38_WORK_MODE','PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_FULL_MODEL_CPU_REFERENCE'):
    check(flags+['-D'+name+'=1'],False)
check(flags,False,include+'#undef '+names[0]+'\n'+include)
check(flags[1:],False,include+'#define '+names[0]+' 1\n'+include)
print('PASS isolated QGWA image root/component roles and late/conflict refusals; NO DEVICE')
