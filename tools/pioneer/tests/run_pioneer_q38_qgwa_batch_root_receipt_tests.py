#!/usr/bin/env python3
"""Connect simulated batched worker proof to a separately compiled root predicate."""
import hashlib
import os
from pathlib import Path
import subprocess
import sys
import tempfile

if len(sys.argv)!=3 or any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ):
    raise SystemExit('absolute QGWA worker and batch leaf directories required')
worker,leaf=map(Path,sys.argv[1:]);here=Path(__file__).resolve().parent;root=here.parent
for path,digest in ((worker/'program.elf','e0d7bb5a1434181a8560867f1bfee701a7787a878dff780bce215adf79f2fc47'),
                    (worker/'globalworker.bin','894747f7bf2a282438932a7dc3b56003d9ec64abcda30914b0fd891b70034207'),
                    (worker/'cachedfpu.bin','46b4b2e86bc7b09383a3945ba5241aa8745d6939c0868d73636fe71de0913753'),
                    (leaf/'text.bin','77d87adaf6ab3f9c34fc9f1c50c627d6174676b700f24bca34c8a28f0bcd1dd0'),
                    (leaf/'boot.bin','d47a1c29c265663f5bd4bd49aaf28248b7345d7feb5a4f770caa27315deaa4dd')):
    if not path.is_absolute() or path.is_symlink() or hashlib.sha256(path.read_bytes()).hexdigest()!=digest:
        raise SystemExit('artifact identity refused: '+str(path))
compiler=os.environ.get('CXX','c++')
root_names=('PIONEER_Q38_QGWA_BATCH_ROOT_RECEIPT_INSPECTION','PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION',
            'PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION','PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT')
root_flags=['-D'+n+'=1' for n in root_names]
role=root/'PioneerQ38QgwaBatchRootReceiptRole.h'
def check(flags,okay,source=None):
    result=subprocess.run([compiler,'-std=c++17',*flags,'-fsyntax-only','-x','c++','-'],
        input=source or '#include "'+str(role)+'"\n',text=True,capture_output=True)
    assert (result.returncode==0)==okay,result.stderr[:1000]
check(root_flags,True)
for name in root_names:
    reduced=[x for x in root_flags if x!='-D'+name+'=1']
    check(reduced,name==root_names[0]);check(reduced+['-D'+name+'=0'],False)
for name in ('_KERNEL_MODE','PIONEER_DRIVER_BUILD_TAG','TT_PIONEER_DRIVER_RELOAD','PIONEER_Q38_TOKEN_NATIVE_CONTEXT',
             'PIONEER_Q38_QGW8_NATIVE_INSPECTION','PIONEER_Q38_HYBRID_FULL_NATIVE','PIONEER_Q38_FULL_OFFLOAD_NATIVE',
             'PIONEER_Q38_HEAD_MATRIX_WORKER_INSPECTION','PIONEER_Q38_HEAD_MATRIX_ROOT_INSPECTION','PIONEER_Q38_QGW8_BFP8_BATCH_WORKER',
             'PIONEER_Q38_QGW8_BFP8_REUSE_WORKER','PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE',
             'PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_FULL_MODEL_CPU_REFERENCE','__FAST_MATH__'):
    check(root_flags+['-D'+name+'=1'],False)
include='#include "'+str(role)+'"\n'
check(root_flags,False,include+'#undef '+root_names[0]+'\n'+include)
check(root_flags[1:],False,include+'#define '+root_names[0]+' 1\n'+include)
worker_names=('PIONEER_Q38_HEAD_MATRIX_WORKER_INSPECTION','PIONEER_Q38_HYBRID_WORKER_IMAGE',
              'PIONEER_Q38_BF16_WORKER_IMAGE','PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION',
              'PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT','PIONEER_Q38_QGW8_BFP8_BATCH_WORKER',
              'PIONEER_Q38_BFP8_BATCH_INPUT_EXPERIMENT','PIONEER_Q38_TEST_BATCH_ROOT_RECEIPT')
worker_flags=['-D'+n+'=1' for n in worker_names]
common=[compiler,'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-ffp-contract=off','-fsanitize=address,undefined']
with tempfile.TemporaryDirectory(prefix='qgwa-root-receipt-',dir='/private/tmp') as directory:
    tmp=Path(directory);obj=tmp/'root.o';old=tmp/'old.o';binary=tmp/'test'
    subprocess.run(common+root_flags+['-c',str(here/'pioneer_q38_qgwa_batch_root_receipt_probe.cpp'),'-o',str(obj)],check=True)
    subprocess.run(common+root_flags[1:]+['-DPIONEER_Q38_TEST_OLD_ROOT_RECEIPT=1','-c',str(here/'pioneer_q38_qgwa_batch_root_receipt_probe.cpp'),'-o',str(old)],check=True)
    subprocess.run(common+worker_flags+[str(here/'pioneer_q38_hybrid_head_worker_image_test.cpp'),str(obj),str(old),'-o',str(binary)],check=True)
    subprocess.run([str(binary),str(worker),str(leaf)],check=True,
                   env=dict(os.environ,ASAN_OPTIONS='quarantine_size_mb=32',UBSAN_OPTIONS='halt_on_error=1'))
print('PASS separate simulated QGWA worker/root receipt, mutations and role refusals; NO ROOT OR DEVICE')
