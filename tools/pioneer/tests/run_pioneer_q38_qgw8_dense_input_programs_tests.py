#!/usr/bin/env python3
"""Test exclusive offline dense-input program DATA, never activate a driver."""
import hashlib
import os
from pathlib import Path
import subprocess
import sys
import tempfile

if len(sys.argv) != 5 or not __debug__ or any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ):
    raise SystemExit('absolute old stage image, two input images and worker required')
old, first, second, worker = map(Path, sys.argv[1:])
if not all(p.is_absolute() and p.is_dir() and not p.is_symlink() for p in (old, first, second, worker)):
    raise SystemExit('invalid artifact directory')
here = Path(__file__).resolve().parent
root = here.parent
generator = root / 'generate_qwen38_hybrid_programs.py'
def generate(*args):
    return subprocess.run([sys.executable, str(generator), *map(str, args)], capture_output=True, text=True)
with tempfile.TemporaryDirectory(prefix='qgw8-dense-input-programs-', dir='/private/tmp') as name:
    tmp = Path(name)
    a, b, prior = tmp/'a.h', tmp/'b.h', tmp/'old.h'
    for image, target in ((first,a),(second,b)):
        result = generate('--qgw8-dense-input-inspection', image, worker, target)
        assert result.returncode == 0 and 'QGW8-DENSE-INPUT' in result.stdout, result.stderr
    data = a.read_bytes()
    assert data == b.read_bytes() and hashlib.sha256(data).hexdigest() == '7c7bcb0b285159e58530357342fb98a920354cb6093c2f6c8ea67e96e1aac523'
    assert b'Family=0x43505451' in data and b'Family==0x43505451' in data
    result = generate('--qgw8-dense-stage-inspection', old, worker, prior)
    assert result.returncode == 0 and hashlib.sha256(prior.read_bytes()).hexdigest() == 'b7ed7030d1ba7de08bafc71fa939a14b7612a2021b2cbba80b6327c76db609f6'
    for selector, image in (('--qgw8-dense-stage-inspection', first), ('--qgw8-layer-trace-inspection', first),
                            ('--qgw8-dense-input-inspection', old)):
        target = tmp / ('refused-' + str(len(list(tmp.iterdir()))) + '.h')
        result = generate(selector, image, tmp/'MUST_NOT_OPEN_WORKER', target)
        assert result.returncode and 'artifact text.bin' in result.stderr and not target.exists()
    target = tmp/'ambiguous.h'
    assert generate('--qgw8-dense-input-inspection','--qgw8-dense-stage-native',first,worker,target).returncode and not target.exists()
    roles = ('PIONEER_Q38_QGW8_DENSE_INPUT_PROGRAMS_INSPECTION','PIONEER_Q38_QGW8_LAYER_TRACE_PROGRAMS_INSPECTION',
             'PIONEER_Q38_QGW8_PROGRAMS_INSPECTION','PIONEER_Q38_QGW8_LAYOUT_INSPECTION',
             'PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION','PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION',
             'PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT')
    flags = ['-D'+r+'=1' for r in roles] + ['-DPIONEER_Q38_HYBRID_PROGRAMS_HEADER="'+str(a)+'"']
    common = [os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-I'+str(root)]
    source = str(here/'pioneer_q38_hybrid_programs_test.cpp')
    subprocess.run(common+flags+['-fsanitize=address,undefined',source,'-o',str(tmp/'test')],check=True)
    subprocess.run([str(tmp/'test'),str(first),str(worker)],check=True,
                   env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
    for role in roles:
        reduced = [f for f in flags if f != '-D'+role+'=1']
        for value in (None,0):
            assert subprocess.run(common+reduced+([] if value is None else ['-D'+role+'=0'])+['-fsyntax-only',source],capture_output=True).returncode,role
    for role in ('_KERNEL_MODE','PIONEER_DRIVER_BUILD_TAG','PIONEER_Q38_QGW8_TRACE_NATIVE_131',
                 'PIONEER_Q38_QGW8_DENSE_STAGE_PROGRAMS_INSPECTION','PIONEER_Q38_QGW8_DENSE_STAGE_NATIVE_132',
                 'PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE','PIONEER_Q38_CPU_VALIDATION',
                 'PIONEER_Q38_FULL_MODEL_CPU_REFERENCE'):
        assert subprocess.run(common+flags+['-D'+role+'=1','-fsyntax-only',source],capture_output=True).returncode,role
print('PASS distinct dense-input DATA, every worker, pinned artifacts and role refusals; NO NATIVE OR DEVICE')
