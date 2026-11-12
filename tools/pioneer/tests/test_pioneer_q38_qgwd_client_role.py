#!/usr/bin/env python3
"""Reject mixed QGWD client roles; no device or model access."""
import os
from pathlib import Path
import subprocess

assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
header=Path(__file__).resolve().parent.parent/'PioneerQ38QgwdClientRole.h'
required=('PIONEER_Q38_QGWD_CLIENT_141','PIONEER_Q38_QGWD_FULL_SESSION_INSPECTION',
 'PIONEER_Q38_QGWD_UPLOAD_SESSION_INSPECTION','PIONEER_Q38_QGW8_FULL_SESSION_INSPECTION',
 'PIONEER_Q38_DENSE_Q8_VERIFIED_MODEL_INSPECTION','PIONEER_Q38_DENSE_Q8_MATRIX_PLAN_INSPECTION')
flags=['-D'+name+'=1' for name in required]
def check(arguments, expected, extra=''):
    probe='#ifndef PIONEER_Q38_QGWD_CLIENT_141\n#error missing selector\n#endif\n#include "'+str(header)+'"\n'+extra
    result=subprocess.run([os.environ.get('CXX','c++'),'-std=c++17',*arguments,'-x','c++','-fsyntax-only','-'],
                          input=probe,text=True,capture_output=True)
    assert (result.returncode==0)==expected,result.stderr[:1000]
check(flags,True)
for name in required:
    selected='-D'+name+'=1';check([f for f in flags if f!=selected],False)
    check([f for f in flags if f!=selected]+['-D'+name+'=0'],False)
for name in ('_KERNEL_MODE','PIONEER_DRIVER_BUILD_TAG','TT_PIONEER_DRIVER_RELOAD',
             'PIONEER_Q38_QGWD_NATIVE_141','PIONEER_Q38_QGWD_NATIVE_INSPECTION',
             'PIONEER_Q38_QGW8_CANDIDATE_CLIENT_128','PIONEER_Q38_QGWC_CLIENT_136',
             'PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE','PIONEER_Q38_CPU_VALIDATION',
             'PIONEER_Q38_FULL_MODEL_CPU_REFERENCE','__FAST_MATH__'):
    check(flags+['-D'+name+'=1'],False)
check(flags,False,'#undef PIONEER_Q38_QGWD_CLIENT_141\n#include "'+str(header)+'"\n')
two=flags+['-DPIONEER_Q38_QGWD_TWO_CLIENT_142=1']
check(two,True)
for name in required:
    selected='-D'+name+'=1';check([f for f in two if f!=selected],False)
for name in ('_KERNEL_MODE','PIONEER_DRIVER_BUILD_TAG','TT_PIONEER_DRIVER_RELOAD',
             'PIONEER_Q38_QGWD_NATIVE_141','PIONEER_Q38_QGWD_TWO_NATIVE_142',
             'PIONEER_Q38_QGWD_MULTI_TOKEN_SIMULATION','PIONEER_Q38_WORK_INFERENCE',
             'TT_PIONEER_Q38_WORK_MODE','PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_FULL_MODEL_CPU_REFERENCE'):
    check(two+['-D'+name+'=1'],False)
check(flags,False,'#define PIONEER_Q38_QGWD_TWO_CLIENT_142 1\n#include "'+str(header)+'"\n')
check(two,False,'#undef PIONEER_Q38_QGWD_TWO_CLIENT_142\n#include "'+str(header)+'"\n')
qgwf=two+['-DPIONEER_Q38_QGWF_CLIENT_143=1'];check(qgwf,True)
for name in required+('PIONEER_Q38_QGWD_TWO_CLIENT_142',):
    selected='-D'+name+'=1';check([f for f in qgwf if f!=selected],False)
for name in ('_KERNEL_MODE','PIONEER_DRIVER_BUILD_TAG','PIONEER_Q38_QGWF_NATIVE_143',
             'PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE','PIONEER_Q38_CPU_VALIDATION',
             'PIONEER_Q38_FULL_MODEL_CPU_REFERENCE'):
    check(qgwf+['-D'+name+'=1'],False)
check(qgwf,False,'#undef PIONEER_Q38_QGWF_CLIENT_143\n#include "'+str(header)+'"\n')
print('PASS QGWD/QGWF client parents and native/Work/reference exclusions; no device')
