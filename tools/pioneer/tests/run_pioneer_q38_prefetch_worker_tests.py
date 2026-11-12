#!/usr/bin/env python3
"""Test an isolated single-job host worker; no device or model."""
import os
from pathlib import Path
import subprocess
import tempfile

here=Path(__file__).resolve().parent
header=here.parent/'PioneerQ38PrefetchWorker.h'
probe='#define PIONEER_Q38_PIPELINED_UPLOAD_EXPERIMENT 1\n#define PIONEER_Q38_PREFETCH_WORKER_UNIT_TEST 1\n#include "'+str(header)+'"\n'
compiler=os.environ.get('CXX','c++')
for forbidden in ('_KERNEL_MODE','PIONEER_DRIVER_BUILD_TAG','PIONEER_Q38_WORK_INFERENCE',
                  'TT_PIONEER_Q38_WORK_MODE','PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_FULL_MODEL_CPU_REFERENCE',
                  'PIONEER_Q38_UPLOAD_TIMING_DIAGNOSTIC','PIONEER_Q38_QGWF_RATE_CLIENT_144'):
    result=subprocess.run([compiler,'-std=c++17','-D'+forbidden+'=1','-E','-x','c++','-'],
                          input=probe,text=True,capture_output=True)
    assert result.returncode,forbidden
with tempfile.TemporaryDirectory(prefix='q38-prefetch-',dir='/private/tmp') as tmp:
    binary=Path(tmp)/'test'
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror',
                    '-fsanitize=address,undefined','-pthread',str(here/'pioneer_q38_prefetch_worker_test.cpp'),
                    '-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True,timeout=45,env=dict(os.environ,UBSAN_OPTIONS='halt_on_error=1'))
    failure=Path(tmp)/'failure'
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror',
                    '-Wno-misleading-indentation','-Wno-unused-function','-Wno-unused-variable',
                    '-fsanitize=address,undefined','-pthread',str(here/'pioneer_q38_pipeline_failure_test.cpp'),
                    str(here.parent/'PioneerModelWeightCache.cpp'),'-o',str(failure)],check=True)
    subprocess.run([str(failure)],check=True,timeout=45,env=dict(os.environ,UBSAN_OPTIONS='halt_on_error=1'))
