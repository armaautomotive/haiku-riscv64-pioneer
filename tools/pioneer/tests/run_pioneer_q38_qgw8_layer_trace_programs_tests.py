#!/usr/bin/env python3
"""Check distinct offline trace program data and native/Work refusals."""
import hashlib
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

if len(sys.argv)!=5 or not __debug__ or any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ):
    raise SystemExit('usage: ABS_BASELINE ABS_TRACE_A ABS_TRACE_B ABS_WORKER')
baseline, first_root, second_root, worker=map(Path,sys.argv[1:])
if not all(p.is_absolute() and p.is_dir() for p in (baseline,first_root,second_root,worker)):
    raise SystemExit('absolute existing input directories required')
here=Path(__file__).resolve().parent; root=here.parent; cxx=os.environ.get('CXX','c++')
generator=root/'generate_qwen38_hybrid_programs.py'
def generate(*args):return subprocess.run([sys.executable,str(generator),*map(str,args)],capture_output=True,text=True)
with tempfile.TemporaryDirectory(prefix='qgw8-trace-programs-',dir='/private/tmp') as name:
    tmp=Path(name); first=tmp/'first.h';second=tmp/'second.h';old=tmp/'old.h'
    for image,out in ((first_root,first),(second_root,second)):
        p=generate('--qgw8-layer-trace-inspection',image,worker,out)
        assert p.returncode==0 and 'QGW8-TRACE' in p.stdout,p.stderr
    header=first.read_bytes();assert header==second.read_bytes()
    assert hashlib.sha256(header).hexdigest()=='60803a2dfe96d9e45ec5da6f71f795b2ca2eb1cf82c013ec1a42841bfb517bd1'
    assert b'Family=0x41505451' in header and b'Family==0x41505451' in header
    p=generate('--qgw8-native-candidate',baseline,worker,old)
    assert p.returncode==0 and hashlib.sha256(old.read_bytes()).hexdigest()=='8ddeb0cfa01857557be9416fa61c8029040570cf67bd412e6dd1f59fdc32eecb'
    for selector,image in (('--qgw8-native-candidate',first_root),('--qgw8-head-inspection',first_root),
                           ('--qgw8-coarse-profile-inspection',first_root),('--qgw8-layer-trace-inspection',baseline)):
        out=tmp/('denied-'+str(len(list(tmp.iterdir())))+'.h')
        p=generate(selector,image,tmp/'MUST_NOT_OPEN_WORKER',out)
        assert p.returncode and 'artifact text.bin' in p.stderr and 'FileNotFoundError' not in p.stderr and not out.exists()
    out=tmp/'ambiguous.h';assert generate('--qgw8-layer-trace-inspection','--qgw8-native-candidate',first_root,worker,out).returncode and not out.exists()
    assert generate('--qgw8-layer-trace-inspection',first_root,worker,first).returncode and first.read_bytes()==header
    damaged=tmp/'damaged';damaged.mkdir()
    sys.path.insert(0,str(root))
    from PioneerQ38Qgw8LayerTraceProgramPins import WORKER_PINS
    for artifact,_,_ in WORKER_PINS:shutil.copyfile(worker/artifact,damaged/artifact)
    p=damaged/'globalworker.bin';data=p.read_bytes();p.write_bytes(bytes([data[0]^1])+data[1:])
    out=tmp/'bad-worker.h';assert generate('--qgw8-layer-trace-inspection',first_root,damaged,out).returncode and not out.exists()
    roles=('PIONEER_Q38_QGW8_LAYER_TRACE_PROGRAMS_INSPECTION','PIONEER_Q38_QGW8_PROGRAMS_INSPECTION',
           'PIONEER_Q38_QGW8_LAYOUT_INSPECTION','PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION',
           'PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION','PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT')
    flags=['-D'+r+'=1' for r in roles]+['-DPIONEER_Q38_HYBRID_PROGRAMS_HEADER="'+str(first)+'"']
    common=[cxx,'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-I'+str(root)]
    source=str(here/'pioneer_q38_hybrid_programs_test.cpp')
    subprocess.run(common+flags+['-fsanitize=address,undefined',source,'-o',str(tmp/'test')],check=True)
    subprocess.run([str(tmp/'test'),str(first_root),str(worker)],check=True,
                   env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
    for role in roles:
        without=[f for f in flags if f!='-D'+role+'=1']
        assert subprocess.run(common+without+['-fsyntax-only',source],capture_output=True).returncode,role
        assert subprocess.run(common+without+['-D'+role+'=0','-fsyntax-only',source],capture_output=True).returncode,role
    for role in ('_KERNEL_MODE','PIONEER_DRIVER_BUILD_TAG','PIONEER_Q38_TOKEN_NATIVE_CONTEXT','TT_PIONEER_DRIVER_RELOAD',
                 'PIONEER_Q38_QGW8_NATIVE_INSPECTION','PIONEER_Q38_BF16_FULL_NATIVE','PIONEER_Q38_BF16_FULL_CLIENT',
                 'PIONEER_Q38_QGW8_LAYER_TRACE_IMAGE','__FAST_MATH__',
                 'PIONEER_Q38_HYBRID_FULL_NATIVE','PIONEER_Q38_FULL_OFFLOAD_NATIVE','PIONEER_Q38_HYBRID_FULL_CLIENT',
                 'PIONEER_Q38_FULL_OFFLOAD_CLIENT','PIONEER_Q38_QGW8_CANDIDATE_NATIVE_128',
                 'PIONEER_Q38_QGW8_CANDIDATE_CLIENT_128','PIONEER_Q38_QGW8_HIDDEN_NATIVE_129',
                 'PIONEER_Q38_QGW8_HIDDEN_CLIENT_129','PIONEER_Q38_QGW8_COARSE_PROGRAMS_INSPECTION',
                 'PIONEER_Q38_HYBRID_NATIVE_COMPILE_ONLY','PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE',
                 'PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_FULL_MODEL_CPU_REFERENCE'):
        assert subprocess.run(common+flags+['-D'+role+'=1','-fsyntax-only',source],capture_output=True).returncode,role
    native=('PIONEER_Q38_QGW8_CANDIDATE_NATIVE_128','_KERNEL_MODE','PIONEER_Q38_QGW8_NATIVE_INSPECTION',
            'PIONEER_Q38_HYBRID_FULL_NATIVE','PIONEER_Q38_FULL_OFFLOAD_NATIVE','PIONEER_Q38_TOKEN_NATIVE_CONTEXT',
            'PIONEER_Q38_HYBRID_TRACKED_UPLOAD','PIONEER_Q38_QGW8_LAYOUT_INSPECTION','PIONEER_Q38_QGW8_PROGRAMS_INSPECTION',
            'PIONEER_Q38_QGW8_CONTROL_INSPECTION','PIONEER_Q38_QGW8_SPAN_INSPECTION','PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION',
            'PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION','PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT')
    native_header='#include "'+str(root/'PioneerQ38Qgw8NativeInspectionRole.h')+'"\n'
    for hidden in (False,True):
        native_flags=['-D'+r+'=1' for r in native]+['-DPIONEER_DRIVER_BUILD_TAG=0x2026110700010'+('129' if hidden else '128')+'ULL']
        if hidden:native_flags+=['-DPIONEER_Q38_QGW8_HIDDEN_NATIVE_129=1','-DTT_PIONEER_DRIVER_RELOAD=1']
        for injected in ([],['-DPIONEER_Q38_QGW8_LAYER_TRACE_PROGRAMS_INSPECTION=0'],['-DPIONEER_Q38_QGW8_LAYER_TRACE_PROGRAMS_INSPECTION=1']):
            result=subprocess.run([cxx,'-std=c++17',*native_flags,*injected,'-fsyntax-only','-x','c++','-'],
                                  input=native_header,text=True,capture_output=True)
            assert (result.returncode==0)==(not injected),(hidden,injected,result.stderr)
    fake=('PIONEER_Q38_QGW8_NATIVE_INSPECTION','_KERNEL_MODE','PIONEER_Q38_HYBRID_NATIVE_COMPILE_ONLY',
          'PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY','PIONEER_Q38_TOKEN_NATIVE_CONTEXT','PIONEER_Q38_HYBRID_TRACKED_UPLOAD',
          'PIONEER_Q38_QGW8_LAYOUT_INSPECTION','PIONEER_Q38_QGW8_PROGRAMS_INSPECTION','PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION',
          'PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION','PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT')
    fake_flags=['-D'+r+'=1' for r in fake]+['-DPIONEER_DRIVER_BUILD_TAG=0x51544b4e4f445256ULL']
    fake_flags+=['-D'+r+'=fake_'+r for r in ('api_version','init_hardware','init_driver','uninit_driver','publish_devices','find_device')]
    for injected in ([],['-DPIONEER_Q38_QGW8_LAYER_TRACE_PROGRAMS_INSPECTION=0'],['-DPIONEER_Q38_QGW8_LAYER_TRACE_PROGRAMS_INSPECTION=1']):
        result=subprocess.run([cxx,'-std=c++17',*fake_flags,*injected,'-fsyntax-only','-x','c++','-'],
                              input=native_header,text=True,capture_output=True)
        assert (result.returncode==0)==(not injected),(injected,result.stderr)
print('PASS distinct trace program DATA, all 120 workers, pins, role and corruption refusals; NO DEVICE OR WORK')
