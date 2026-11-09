#!/usr/bin/env python3
"""Validate all dense Q8 BFP8 job geometry; no device or model data."""
import os
from pathlib import Path
import subprocess
import tempfile
if any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ):raise SystemExit('role override refused')
here=Path(__file__).resolve().parent
with tempfile.TemporaryDirectory(prefix='q38-dense-q8-jobs-',dir='/private/tmp') as directory:
    binary=Path(directory)/'test'
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror',
                    '-Wno-misleading-indentation','-Wno-unused-function','-Wno-unused-variable',
                    '-fsanitize=address,undefined',str(here/'pioneer_q38_dense_q8_matrix_jobs_test.cpp'),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True,env=dict(os.environ,UBSAN_OPTIONS='halt_on_error=1'))
    header=here.parent/'PioneerQ38DenseQ8MatrixJobs.h'
    base=['-DPIONEER_Q38_DENSE_Q8_MATRIX_PLAN_INSPECTION=1']
    for role in ('_KERNEL_MODE','PIONEER_DRIVER_BUILD_TAG','PIONEER_Q38_TOKEN_NATIVE_CONTEXT',
                 'PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE','PIONEER_Q38_CPU_VALIDATION',
                 'PIONEER_Q38_FULL_MODEL_CPU_REFERENCE'):
        result=subprocess.run([os.environ.get('CXX','c++'),'-std=c++17',*base,'-D'+role+'=1',
                               '-fsyntax-only','-x','c++',str(header)],capture_output=True)
        assert result.returncode and b'Dense Q8 matrix jobs are offline geometry only' in result.stderr,role
print('PASS full dense Q8 matrix job geometry; SYNTHETIC NO DEVICE')
