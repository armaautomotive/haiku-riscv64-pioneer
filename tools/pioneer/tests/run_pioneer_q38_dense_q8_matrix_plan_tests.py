#!/usr/bin/env python3
"""Exercise dense Q8 matrix layout only; no device or model access."""
import os
from pathlib import Path
import subprocess
import tempfile
here=Path(__file__).resolve().parent;root=here.parent;source=here/'pioneer_q38_dense_q8_matrix_plan_test.cpp'
if any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ):raise SystemExit('role override refused')
with tempfile.TemporaryDirectory(prefix='q38-dense-q8-matrix-plan-',dir='/private/tmp') as directory:
    binary=Path(directory)/'test'
    common=[os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-Wno-misleading-indentation','-Wno-unused-function']
    subprocess.run(common+['-fsanitize=address,undefined',str(source),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True,env=dict(os.environ,UBSAN_OPTIONS='halt_on_error=1'))
    header=root/'PioneerQ38HybridDenseQ8MatrixPlan.h'
    required=['-DPIONEER_Q38_DENSE_Q8_MATRIX_PLAN_INSPECTION=1','-DPIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION=1',
              '-DPIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION=1','-DPIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT=1']
    subprocess.run(common+required+['-fsyntax-only','-x','c++',str(header)],check=True)
    for forbidden in ('_KERNEL_MODE','PIONEER_DRIVER_BUILD_TAG','PIONEER_Q38_TOKEN_NATIVE_CONTEXT',
                      'PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE',
                      'PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_FULL_MODEL_CPU_REFERENCE'):
        result=subprocess.run(common+required+['-D'+forbidden+'=1','-fsyntax-only','-x','c++',str(header)],capture_output=True)
        assert result.returncode,forbidden
    for value in (None,0):
        args=[f for f in required if not f.startswith('-DPIONEER_Q38_DENSE_Q8_MATRIX_PLAN_INSPECTION=')]
        if value is not None:args+=['-DPIONEER_Q38_DENSE_Q8_MATRIX_PLAN_INSPECTION=0']
        assert subprocess.run(common+args+['-fsyntax-only','-x','c++',str(header)],capture_output=True).returncode
print('PASS dense Q8 matrix geometry and exclusive offline role; NO DEVICE')
