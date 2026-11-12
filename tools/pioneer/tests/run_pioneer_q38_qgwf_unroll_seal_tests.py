#!/usr/bin/env python3
"""Compare pinned actual RV32 seal instructions; synthetic endpoints only."""
import hashlib
import os
from pathlib import Path
import subprocess
import sys
import tempfile

assert len(sys.argv)==4 and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
baseline,candidate,leaf=map(Path,sys.argv[1:]);here=Path(__file__).resolve().parent
for folder,digest in ((baseline,'c6f884e7a594d84832ce6691836ea10e5cffd10abf64ddab08f2b2023e462c0e'),
                      (candidate,'14f5f416836f5dee5a07c400952cb0efe8d64ce70edf5d2112ef855632f4cc0d')):
    assert folder.is_absolute() and folder.is_dir() and hashlib.sha256((folder/'program.elf').read_bytes()).hexdigest()==digest
assert leaf.is_absolute() and hashlib.sha256((leaf/'text.bin').read_bytes()).hexdigest()=='2942f4c427b17592d3149f3fcaf5cc197f64570702033189390749a851da51c9'
names=('PIONEER_Q38_QGWF_BATCH4_WORKER','PIONEER_Q38_QGWE_BATCH_DENSE_WORKER','PIONEER_Q38_QGW8_BFP8_BATCH_WORKER',
       'PIONEER_Q38_BFP4_BATCH_INPUT_EXPERIMENT','PIONEER_Q38_QGWD_SRCA_DENSE_Q8_WORKER',
       'PIONEER_Q38_DENSE_Q8_MATRIX_WORKER_INSPECTION','PIONEER_Q38_DENSE_BF16_SRCA_BASE_CANDIDATE',
       'PIONEER_Q38_HEAD_MATRIX_WORKER_INSPECTION','PIONEER_Q38_HYBRID_WORKER_IMAGE','PIONEER_Q38_BF16_WORKER_IMAGE',
       'PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION','PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT')
flags=['-D'+n+'=1' for n in names];compiler=os.environ.get('CXX','c++')
source=here.parent/'blackhole_qwen38_global_worker.cpp'
for forbidden in ('_KERNEL_MODE','PIONEER_DRIVER_BUILD_TAG','PIONEER_Q38_WORK_INFERENCE',
                  'TT_PIONEER_Q38_WORK_MODE','PIONEER_Q38_HYBRID_FULL_NATIVE',
                  'PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_FULL_MODEL_CPU_REFERENCE','__FAST_MATH__'):
    result=subprocess.run([compiler,'-std=c++17','-E',*flags,'-DPIONEER_Q38_QGWF_UNROLL_SEAL_EXPERIMENT=1',
                           '-D'+forbidden+'=1',str(source)],capture_output=True)
    assert result.returncode,forbidden
with tempfile.TemporaryDirectory(prefix='qgwf-seal-',dir='/private/tmp') as tmp:
    binary=Path(tmp)/'seal'
    subprocess.run([compiler,'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-Wno-unused-function',
                    '-Wno-unused-variable','-ffp-contract=off','-fsanitize=address,undefined',*flags,
                    str(here/'pioneer_q38_qgwf_unroll_seal_test.cpp'),'-o',str(binary)],check=True)
    subprocess.run([str(binary),str(baseline),str(candidate),str(leaf)],check=True,
                   env=dict(os.environ,ASAN_OPTIONS='quarantine_size_mb=32',UBSAN_OPTIONS='halt_on_error=1'))
