#!/usr/bin/env python3
"""Authenticate the QGWA ROOT/worker code DATA; never load a device."""
import hashlib
import os
from pathlib import Path
import subprocess
import sys
import tempfile

if len(sys.argv)!=7 or any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ):
    raise SystemExit('QGWA_ROOT_A QGWA_ROOT_B QGWA_WORKER_A QGWA_WORKER_B QGW8_ROOT QGW8_WORKER required')
first,second,worker1,worker2,oldroot,oldworker=map(Path,sys.argv[1:])
if not all(p.is_absolute() and p.is_dir() and not p.is_symlink() for p in (first,second,worker1,worker2,oldroot,oldworker)):
    raise SystemExit('invalid artifact directory')
here=Path(__file__).resolve().parent;root=here.parent;generator=root/'generate_qwen38_hybrid_programs.py'
def generate(*args):return subprocess.run([sys.executable,str(generator),*map(str,args)],capture_output=True,text=True)
with tempfile.TemporaryDirectory(prefix='qgwa-batch-programs-',dir='/private/tmp') as directory:
    tmp=Path(directory);a=tmp/'a.h';b=tmp/'b.h';old=tmp/'old.h'
    for image,worker,target in ((first,worker1,a),(second,worker2,b)):
        result=generate('--qgwa-batch-inspection',image,worker,target)
        assert result.returncode==0 and 'QGWA-BATCH' in result.stdout,result.stderr
    data=a.read_bytes()
    assert data==b.read_bytes() and hashlib.sha256(data).hexdigest()=='b615fda5f32852814f5383c6ec66c7a80a4c344cd4e7e196a593825beac462d8'
    assert b'Family=0x44505451' in data and b'Family==0x44505451' in data
    result=generate('--qgw8-head-inspection',oldroot,oldworker,old)
    assert result.returncode==0,result.stderr
    for selector,image,worker in (('--qgw8-head-inspection',first,worker1),
                                  ('--qgwa-batch-inspection',oldroot,oldworker),
                                  ('--qgwa-batch-inspection',first,oldworker),
                                  ('--qgwa-batch-inspection',oldroot,worker1)):
        target=tmp/('refused-'+str(len(list(tmp.iterdir())))+'.h')
        assert generate(selector,image,worker,target).returncode and not target.exists()
    target=tmp/'ambiguous.h'
    assert generate('--qgwa-batch-inspection','--qgw8-dense-input-native',first,worker1,target).returncode and not target.exists()
    roles=('PIONEER_Q38_QGWA_BATCH_PROGRAMS_INSPECTION','PIONEER_Q38_QGW8_PROGRAMS_INSPECTION',
           'PIONEER_Q38_QGW8_LAYOUT_INSPECTION','PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION',
           'PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION','PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT')
    flags=['-D'+n+'=1' for n in roles]+['-DPIONEER_Q38_HYBRID_PROGRAMS_HEADER="'+str(a)+'"']
    common=[os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-I'+str(root)]
    source=str(here/'pioneer_q38_hybrid_programs_test.cpp');binary=tmp/'test'
    subprocess.run(common+flags+['-fsanitize=address,undefined',source,'-o',str(binary)],check=True)
    subprocess.run([str(binary),str(first),str(worker1)],check=True,
                   env=dict(os.environ,ASAN_OPTIONS='quarantine_size_mb=32',UBSAN_OPTIONS='halt_on_error=1'))
    # The generic QGW8 program header does not itself require PLAN/LAYOUT;
    # the distinct QGWA selector and generic QGW8 selector must be present.
    for role in roles[:2]:
        reduced=[f for f in flags if f!='-D'+role+'=1']
        for value in (None,0):
            assert subprocess.run(common+reduced+([] if value is None else ['-D'+role+'=0'])+['-fsyntax-only',source],capture_output=True).returncode,role
    for name in ('_KERNEL_MODE','PIONEER_DRIVER_BUILD_TAG','TT_PIONEER_DRIVER_RELOAD',
                 'PIONEER_Q38_TOKEN_NATIVE_CONTEXT','PIONEER_Q38_HYBRID_FULL_NATIVE',
                 'PIONEER_Q38_QGWA_BATCH_ROOT_IMAGE','PIONEER_Q38_QGW8_BFP8_BATCH_WORKER',
                 'PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE','PIONEER_Q38_CPU_VALIDATION',
                 'PIONEER_Q38_FULL_MODEL_CPU_REFERENCE'):
        assert subprocess.run(common+flags+['-D'+name+'=1','-fsyntax-only',source],capture_output=True).returncode,name
print('PASS distinct QGWA DATA, every worker and role/identity refusals; NO NATIVE OR DEVICE')
