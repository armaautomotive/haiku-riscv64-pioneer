#!/usr/bin/env python3
"""Host-only bulk loader byte equivalence and role-exclusion checks."""
import os
from pathlib import Path
import subprocess
import tempfile

here=Path(__file__).resolve().parent;root=here.parent;compiler=os.environ.get('CXX','c++')
with tempfile.TemporaryDirectory(prefix='q38-bfp4-bulk-',dir='/private/tmp') as directory:
 binary=Path(directory)/'bulk-test';source=here/'pioneer_q38_bfp4_bulk_worker_loader_test.cpp'
 subprocess.run([compiler,'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-Wno-unused-function',
  '-fsanitize=address,undefined',str(source),'-o',str(binary)],check=True)
 subprocess.run([str(binary)],check=True)
 header=root/'PioneerQ38Bfp4BulkWorkerLoader.h'
 for forbidden in ('_KERNEL_MODE','PIONEER_DRIVER_BUILD_TAG','PIONEER_Q38_HYBRID_WORKER_IMAGE',
                   '__FAST_MATH__','PIONEER_Q38_BFP4_INPUT_REUSE_EXPERIMENT',
                   'PIONEER_Q38_BFP4_INPUT_REUSE_CHECKSUM_EXPERIMENT','PIONEER_Q38_HYBRID_FULL_CLIENT',
                   'PIONEER_Q38_HYBRID_FULL_NATIVE','PIONEER_Q38_FULL_OFFLOAD_CLIENT',
                   'PIONEER_Q38_FULL_OFFLOAD_NATIVE','PIONEER_Q38_BF16_FULL_CLIENT','PIONEER_Q38_BF16_FULL_NATIVE',
                   'PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE',
                   'PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_FULL_MODEL_CPU_REFERENCE'):
  result=subprocess.run([compiler,'-std=c++17','-DPIONEER_Q38_BFP4_WORKER_DIAGNOSTIC=1',
   '-DPIONEER_Q38_BFP4_BULK_LOAD_EXPERIMENT=1','-D'+forbidden+'=1','-E','-x','c++',str(header)],
   capture_output=True,text=True)
  assert result.returncode and 'BFP4 bulk loading is an isolated offline experiment' in result.stderr,forbidden
 for missing in ([],['-DPIONEER_Q38_BFP4_BULK_LOAD_EXPERIMENT=0']):
  result=subprocess.run([compiler,'-std=c++17','-DPIONEER_Q38_BFP4_WORKER_DIAGNOSTIC=1',*missing,
   '-E','-x','c++',str(header)],capture_output=True,text=True)
  assert result.returncode and 'BFP4 bulk loading is an isolated offline experiment' in result.stderr
 print('PASS BFP4 bulk transport host simulation and role refusals; no native code or card qualification')
