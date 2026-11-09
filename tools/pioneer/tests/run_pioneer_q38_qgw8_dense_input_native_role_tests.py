#!/usr/bin/env python3
"""Test tag133's build-only identity and code selection; no driver or device."""
import hashlib
import os
from pathlib import Path
import subprocess
import sys

if len(sys.argv)!=2 or any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ):
    raise SystemExit('absolute native program header required')
header=Path(sys.argv[1])
if not header.is_absolute() or header.is_symlink() or hashlib.sha256(header.read_bytes()).hexdigest()!='81cc1a2be97a4246d7ae9a9a7e9594e589f8af795d49912031f0d776c754e412':
    raise SystemExit('native program identity refused')
here=Path(__file__).resolve().parent;root=here.parent;compiler=os.environ.get('CXX','c++')
roles='''PIONEER_Q38_QGW8_TRACE_NATIVE_131 PIONEER_Q38_QGW8_DENSE_INPUT_NATIVE_133 _KERNEL_MODE
 PIONEER_Q38_QGW8_CANDIDATE_NATIVE_128 PIONEER_Q38_QGW8_NATIVE_INSPECTION PIONEER_Q38_HYBRID_FULL_NATIVE
 PIONEER_Q38_FULL_OFFLOAD_NATIVE PIONEER_Q38_TOKEN_NATIVE_CONTEXT PIONEER_Q38_QGW8_PROGRAMS_INSPECTION
 PIONEER_Q38_QGW8_LAYOUT_INSPECTION PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION
 PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT
 TT_PIONEER_DRIVER_RELOAD'''.split()
flags=['-D'+name+'=1' for name in roles]+['-DPIONEER_DRIVER_BUILD_TAG=0x2026110700010133ULL']
role=root/'PioneerQ38Qgw8TraceNativeProgramRole.h'
reload=here.parents[2]/'src/add-ons/kernel/drivers/misc/PioneerDriverReloadProtocol.h'
def check(path,options,expected):
    p=subprocess.run([compiler,'-std=c++17','-I'+str(root),*options,'-fsyntax-only','-x','c++',str(path)],capture_output=True)
    assert (p.returncode==0)==expected,p.stderr.decode(errors='replace')[:1000]
check(role,flags,True);check(reload,flags,True)
for name in roles:
    reduced=[f for f in flags if f!='-D'+name+'=1']
    check(role,reduced,False)
for name in ('PIONEER_Q38_QGW8_DENSE_INPUT_NATIVE_133','PIONEER_Q38_QGW8_TRACE_NATIVE_131','TT_PIONEER_DRIVER_RELOAD'):
    reduced=[f for f in flags if f!='-D'+name+'=1']
    check(reload,reduced,False);check(reload,reduced+['-D'+name+'=0'],False)
for name in ('PIONEER_Q38_QGW8_DENSE_STAGE_NATIVE_132','PIONEER_Q38_QGW8_DENSE_INPUT_CLIENT_133',
             'PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE','PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_FULL_MODEL_CPU_REFERENCE'):
    check(role,flags+['-D'+name+'=1'],False)
for tag in ('0x2026110700010131ULL','0x2026110700010132ULL'):
    other=[f for f in flags if not f.startswith('-DPIONEER_DRIVER_BUILD_TAG=')]+['-DPIONEER_DRIVER_BUILD_TAG='+tag]
    check(role,other,False);check(reload,other,False)
selected=root/'PioneerQ38TokenSelectedPrograms.h'
extras=['-DPIONEER_Q38_HYBRID_PROGRAMS_DIAGNOSTIC=1','-DPIONEER_Q38_HYBRID_JOURNAL_DIAGNOSTIC=1',
        '-DPIONEER_Q38_TOKEN_BF16_PROGRAMS=1','-DPIONEER_Q38_HYBRID_TRACKED_UPLOAD=1',
        '-DPIONEER_Q38_QGW8_CONTROL_INSPECTION=1','-DPIONEER_Q38_QGW8_SPAN_INSPECTION=1',
        '-DPIONEER_Q38_HYBRID_PROGRAMS_HEADER="'+str(header)+'"']
check(selected,flags+extras,True)
check(selected,[f for f in flags if f!='-DPIONEER_Q38_QGW8_DENSE_INPUT_NATIVE_133=1']+extras,False)
oldflags=[f for f in flags if f not in ('-DPIONEER_Q38_QGW8_DENSE_INPUT_NATIVE_133=1',
                                        '-DPIONEER_DRIVER_BUILD_TAG=0x2026110700010133ULL')]
oldflags+=['-DPIONEER_Q38_QGW8_DENSE_STAGE_NATIVE_132=1','-DPIONEER_DRIVER_BUILD_TAG=0x2026110700010132ULL']
check(selected,oldflags+extras,False)
print('PASS tag133 native role, exact reload tag and QTPC selection/refusals; BUILD ONLY, NO DRIVER OR DEVICE')
