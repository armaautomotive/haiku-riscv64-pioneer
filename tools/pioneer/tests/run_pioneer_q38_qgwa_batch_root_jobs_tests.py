#!/usr/bin/env python3
"""Simulate actual QGWA ROOT job construction and worker handoff; no device."""
import hashlib
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

if len(sys.argv)!=6 or any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ):
    raise SystemExit('absolute SDK, QGWA root, old root, worker and leaf required')
sdk,root,old,worker,leaf=map(Path,sys.argv[1:]);here=Path(__file__).resolve().parent
if not sdk.is_absolute() or not sdk.is_dir():raise SystemExit('invalid SDK')
# This shared test pins every executed ROOT/worker/leaf artifact and first
# checks that the actual emitted receipt rejects old worker identities.
subprocess.run([sys.executable,str(here/'run_pioneer_q38_qgwa_linked_root_receipt_tests.py'),
                str(worker),str(leaf),str(root),str(old)],check=True)
if hashlib.sha256((root/'program.elf').read_bytes()).hexdigest()!='99a0dc86b01efa012323b8602668e74b757547927a60a1a4b8808df76a9c07de':
    raise SystemExit('root identity refused')
compiler=os.environ.get('CXX','c++');tool=str(sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku-')
common=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-Wno-unused-function','-ffp-contract=off','-fsanitize=address,undefined']
batch=['-DPIONEER_Q38_QGWA_BATCH_ROOT_RECEIPT_INSPECTION=1']
sim=['-DPIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION=1','-DPIONEER_Q38_HEAD_MATRIX_SIMULATION=1',
     '-DPIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION=1','-DPIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT=1',*batch]
worker_roles=['-D'+name+'=1' for name in ('PIONEER_Q38_HEAD_MATRIX_WORKER_INSPECTION','PIONEER_Q38_HYBRID_WORKER_IMAGE',
 'PIONEER_Q38_BF16_WORKER_IMAGE','PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION','PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT',
 'PIONEER_Q38_QGW8_BFP8_BATCH_WORKER','PIONEER_Q38_BFP8_BATCH_INPUT_EXPERIMENT')]
env=dict(os.environ,ASAN_OPTIONS='quarantine_size_mb=32',UBSAN_OPTIONS='halt_on_error=1')
def execute(args,skip_legacy_counts=False):
    result=subprocess.run([str(x) for x in args],check=True,env=env,capture_output=True,text=True)
    if result.stderr:raise RuntimeError('simulation stderr: '+result.stderr)
    for line in result.stdout.splitlines():
        # This shared helper reports static read counts for the older QGW8
        # loader. They do not describe this batched QGWA worker.
        if skip_legacy_counts and line.startswith('QGW8_HEAD_STATIC_LOAD_COUNTS '):continue
        print(line.replace('QGW8','QGWA'))
with tempfile.TemporaryDirectory(prefix='qgwa-root-jobs-',dir='/private/tmp') as directory:
    tmp=Path(directory);image=tmp/'image';image.mkdir()
    for name in ('boot.bin','text.bin','terminal.bin','conv.bin','bf16.bin','program.elf'):shutil.copyfile(root/name,image/name)
    (image/'program.demangled').write_bytes(subprocess.check_output([tool+'nm','-nC',str(image/'program.elf')]))
    outputs={}
    for name,source,flags in (('plan','pioneer_q38_hybrid_head_plan_test.cpp',batch),
                              ('actual','pioneer_q38_hybrid_head_actual_root_test.cpp',sim),
                              ('helpers','pioneer_q38_hybrid_head_root_helpers_test.cpp',sim),
                              ('gather','pioneer_q38_hybrid_head_gather_test.cpp',batch),
                              ('worker','pioneer_q38_hybrid_head_worker_image_test.cpp',worker_roles)):
        binary=tmp/name;outputs[name]=binary
        subprocess.run([compiler,*common,*flags,str(here/source),'-o',str(binary)],check=True)
    job,manifest,metadata,proof=(tmp/n for n in ('job','manifest','metadata','proof'))
    execute((outputs['plan'],job,manifest,metadata),skip_legacy_counts=True)
    execute((outputs['worker'],worker,leaf,job,proof))
    execute((outputs['actual'],image,manifest))
    execute((outputs['helpers'],image,metadata,job,proof))
    execute((outputs['gather'],image,metadata,proof))
print('PASS QGWA emitted ROOT jobs, head handoff, worker proof and synthetic Gather; NO full lifecycle, hardware or TPS')
