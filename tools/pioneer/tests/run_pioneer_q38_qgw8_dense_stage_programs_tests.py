#!/usr/bin/env python3
"""Authenticate distinct offline dense-stage code DATA; never load it."""
import hashlib
import os
from pathlib import Path
import subprocess
import sys
import tempfile

if len(sys.argv)!=5 or not __debug__ or any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ):
    raise SystemExit('absolute original trace, two dense-stage roots and worker required')
old, first, second, worker=map(Path,sys.argv[1:])
if not all(p.is_absolute() and p.is_dir() and not p.is_symlink() for p in (old,first,second,worker)):
    raise SystemExit('invalid artifact directory')
here=Path(__file__).resolve().parent; root=here.parent; generator=root/'generate_qwen38_hybrid_programs.py'
def generate(*args):
    return subprocess.run([sys.executable,str(generator),*map(str,args)],capture_output=True,text=True)
with tempfile.TemporaryDirectory(prefix='qgw8-dense-stage-programs-',dir='/private/tmp') as name:
    tmp=Path(name); a=tmp/'a.h';b=tmp/'b.h';prior=tmp/'old.h'
    for image,out in ((first,a),(second,b)):
        result=generate('--qgw8-dense-stage-inspection',image,worker,out)
        assert result.returncode==0 and 'QGW8-DENSE-STAGE' in result.stdout,result.stderr
    data=a.read_bytes();assert data==b.read_bytes()
    assert hashlib.sha256(data).hexdigest()=='b7ed7030d1ba7de08bafc71fa939a14b7612a2021b2cbba80b6327c76db609f6'
    assert b'Family=0x42505451' in data and b'Family==0x42505451' in data
    result=generate('--qgw8-layer-trace-inspection',old,worker,prior)
    assert result.returncode==0 and hashlib.sha256(prior.read_bytes()).hexdigest()=='60803a2dfe96d9e45ec5da6f71f795b2ca2eb1cf82c013ec1a42841bfb517bd1'
    for selector,image in (('--qgw8-layer-trace-inspection',first),('--qgw8-layer-trace-native',first),
                           ('--qgw8-dense-stage-inspection',old)):
        out=tmp/('refused-'+str(len(list(tmp.iterdir())))+'.h')
        result=generate(selector,image,tmp/'MUST_NOT_OPEN_WORKER',out)
        assert result.returncode and 'artifact text.bin' in result.stderr and not out.exists()
    out=tmp/'ambiguous.h'
    assert generate('--qgw8-dense-stage-inspection','--qgw8-layer-trace-native',first,worker,out).returncode and not out.exists()
    roles=('PIONEER_Q38_QGW8_DENSE_STAGE_PROGRAMS_INSPECTION','PIONEER_Q38_QGW8_LAYER_TRACE_PROGRAMS_INSPECTION',
           'PIONEER_Q38_QGW8_PROGRAMS_INSPECTION','PIONEER_Q38_QGW8_LAYOUT_INSPECTION',
           'PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION','PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION',
           'PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT')
    flags=['-D'+r+'=1' for r in roles]+['-DPIONEER_Q38_HYBRID_PROGRAMS_HEADER="'+str(a)+'"']
    common=[os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-I'+str(root)]
    source=str(here/'pioneer_q38_hybrid_programs_test.cpp')
    subprocess.run(common+flags+['-fsanitize=address,undefined',source,'-o',str(tmp/'test')],check=True)
    subprocess.run([str(tmp/'test'),str(first),str(worker)],check=True,
                   env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
    for role in roles:
        reduced=[f for f in flags if f!='-D'+role+'=1']
        assert subprocess.run(common+reduced+['-fsyntax-only',source],capture_output=True).returncode,role
        assert subprocess.run(common+reduced+['-D'+role+'=0','-fsyntax-only',source],capture_output=True).returncode,role
    for role in ('_KERNEL_MODE','PIONEER_DRIVER_BUILD_TAG','PIONEER_Q38_QGW8_TRACE_NATIVE_131',
                 'PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE','PIONEER_Q38_CPU_VALIDATION',
                 'PIONEER_Q38_FULL_MODEL_CPU_REFERENCE'):
        assert subprocess.run(common+flags+['-D'+role+'=1','-fsyntax-only',source],capture_output=True).returncode,role
print('PASS distinct dense-stage program DATA, every worker, pinned artifacts and role refusals; NO NATIVE OR DEVICE')
