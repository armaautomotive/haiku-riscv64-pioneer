#!/usr/bin/env python3
"""Compile-only checks for the distinct trace client selection."""
import os
from pathlib import Path
import subprocess

assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
role=Path(__file__).resolve().parent.parent/'PioneerQ38Qgw8TraceClientRole.h'
required='''PIONEER_Q38_QGW8_TRACE_CLIENT_131 PIONEER_Q38_QGW8_CANDIDATE_CLIENT_128
PIONEER_Q38_QGW8_FULL_SESSION_INSPECTION PIONEER_Q38_QGW8_USER_SESSION_INSPECTION
PIONEER_Q38_HYBRID_UPLOAD_DIAGNOSTIC PIONEER_Q38_HYBRID_HEAD_UPLOAD_DIAGNOSTIC PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION
PIONEER_Q38_QGW8_LAYOUT_INSPECTION PIONEER_Q38_QGW8_CONTROL_INSPECTION
PIONEER_Q38_QGW8_SPAN_INSPECTION PIONEER_Q38_HYBRID_TRACKED_UPLOAD'''.split()
flags=['-D'+name+'=1' for name in required]
compiler=os.environ.get('CXX','c++')
include='#include "'+str(role)+'"\n'
def check(options,okay,source=include):
    result=subprocess.run([compiler,'-std=c++17',*options,'-fsyntax-only','-x','c++','-'],input=source,text=True,capture_output=True)
    assert (result.returncode==0)==okay,result.stderr[:1000]
check(flags,True)
for name in required:
    flag='-D'+name+'=1'
    if name!=required[0]:check([x for x in flags if x!=flag],False)
    check([x for x in flags if x!=flag]+['-D'+name+'=0'],False)
for name in '''_KERNEL_MODE PIONEER_DRIVER_BUILD_TAG TT_PIONEER_DRIVER_RELOAD PIONEER_Q38_TOKEN_NATIVE_CONTEXT PIONEER_Q38_QGW8_TRACE_NATIVE_131
PIONEER_Q38_QGW8_CANDIDATE_NATIVE_128 PIONEER_Q38_QGW8_HIDDEN_NATIVE_129 PIONEER_Q38_QGW8_HIDDEN_CLIENT_129
PIONEER_Q38_QGW8_TRACE_INIT_INSPECTION PIONEER_Q38_QGW8_TRACE_INIT_DATA_INSPECTION
PIONEER_Q38_FULL_OFFLOAD_NATIVE PIONEER_Q38_FULL_OFFLOAD_CLIENT PIONEER_Q38_HYBRID_FULL_NATIVE
PIONEER_Q38_HYBRID_FULL_CLIENT PIONEER_Q38_BF16_FULL_NATIVE PIONEER_Q38_BF16_FULL_CLIENT
PIONEER_Q38_WORK_INFERENCE TT_PIONEER_Q38_WORK_MODE PIONEER_Q38_CPU_VALIDATION PIONEER_Q38_FULL_MODEL_CPU_REFERENCE'''.split():
    for value in (0,1):check(flags+['-D'+name+'='+str(value)],False)
check(flags,False,include+'#undef PIONEER_Q38_QGW8_TRACE_CLIENT_131\n'+include)
check(flags[1:],False,include+'#define PIONEER_Q38_QGW8_TRACE_CLIENT_131 1\n'+include)
stage=flags+['-DPIONEER_Q38_QGW8_DENSE_STAGE_CLIENT_132=1']
check(stage,True)
check(flags+['-DPIONEER_Q38_QGW8_DENSE_STAGE_CLIENT_132=0'],False)
check([f for f in stage if f!='-DPIONEER_Q38_QGW8_TRACE_CLIENT_131=1'],False)
check(stage+['-DPIONEER_Q38_QGW8_DENSE_STAGE_NATIVE_132=1'],False)
check(stage,False,include+'#undef PIONEER_Q38_QGW8_DENSE_STAGE_CLIENT_132\n'+include)
check(flags,False,include+'#define PIONEER_Q38_QGW8_DENSE_STAGE_CLIENT_132 1\n'+include)
input_role=flags+['-DPIONEER_Q38_QGW8_DENSE_INPUT_CLIENT_133=1']
check(input_role,True)
check(flags+['-DPIONEER_Q38_QGW8_DENSE_INPUT_CLIENT_133=0'],False)
check([f for f in input_role if f!='-DPIONEER_Q38_QGW8_TRACE_CLIENT_131=1'],False)
for forbidden in ('PIONEER_Q38_QGW8_DENSE_STAGE_NATIVE_132','PIONEER_Q38_QGW8_DENSE_STAGE_CLIENT_132',
                  'PIONEER_Q38_QGW8_DENSE_INPUT_NATIVE_133','PIONEER_Q38_WORK_INFERENCE','PIONEER_Q38_CPU_VALIDATION'):
    check(input_role+['-D'+forbidden+'=1'],False)
check(input_role,False,include+'#undef PIONEER_Q38_QGW8_DENSE_INPUT_CLIENT_133\n'+include)
check(flags,False,include+'#define PIONEER_Q38_QGW8_DENSE_INPUT_CLIENT_133 1\n'+include)
value_role=flags+['-DPIONEER_Q38_QGW8_DENSE_VALUE_CLIENT_134=1']
check(value_role,True)
check(flags+['-DPIONEER_Q38_QGW8_DENSE_VALUE_CLIENT_134=0'],False)
for forbidden in ('PIONEER_Q38_QGW8_DENSE_VALUE_NATIVE_134','PIONEER_Q38_QGW8_DENSE_INPUT_NATIVE_133',
                  'PIONEER_Q38_QGW8_DENSE_INPUT_CLIENT_133','PIONEER_Q38_QGW8_DENSE_STAGE_NATIVE_132',
                  'PIONEER_Q38_QGW8_DENSE_STAGE_CLIENT_132','PIONEER_Q38_WORK_INFERENCE','PIONEER_Q38_CPU_VALIDATION'):
    check(value_role+['-D'+forbidden+'=1'],False)
check(value_role,False,include+'#undef PIONEER_Q38_QGW8_DENSE_VALUE_CLIENT_134\n'+include)
check(flags,False,include+'#define PIONEER_Q38_QGW8_DENSE_VALUE_CLIENT_134 1\n'+include)
print('PASS isolated tag133 client role and late-selection refusals; NO SESSION OR DEVICE')
print('PASS isolated tag132 client role and late-selection refusals; NO SESSION OR DEVICE')
print('PASS isolated tag131 client role: parent, conflict and late-selection refusals; NO SESSION OR DEVICE')
