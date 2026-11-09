#!/usr/bin/env python3
"""Test proposed dense-Q8 journal geometry; no device or native owner."""
import os
from pathlib import Path
import subprocess
import tempfile
if any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ):raise SystemExit('role override refused')
here=Path(__file__).resolve().parent;compiler=os.environ.get('CXX','c++')
with tempfile.TemporaryDirectory(prefix='q38-q8-footprint-',dir='/private/tmp') as directory:
    binary=Path(directory)/'test'
    subprocess.run([compiler,'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror',
                    '-fsanitize=address,undefined',str(here/'pioneer_q38_dense_q8_footprint_test.cpp'),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True,env=dict(os.environ,UBSAN_OPTIONS='halt_on_error=1'))
    header=here.parent/'PioneerQ38DenseQ8TokenFootprint.h'
    flags=['-DPIONEER_Q38_DENSE_Q8_MATRIX_FOOTPRINT_INSPECTION=1','-DPIONEER_Q38_HYBRID_JOURNAL_DIAGNOSTIC=1']
    subprocess.run([compiler,'-std=c++17',*flags,'-fsyntax-only','-x','c++',str(header)],check=True)
    for flag in flags:
        for replacement in ([],[flag[:-1]+'0']):
            result=subprocess.run([compiler,'-std=c++17',*[f for f in flags if f!=flag],*replacement,
                                   '-fsyntax-only','-x','c++',str(header)],capture_output=True)
            assert result.returncode and b'isolated offline inspection only' in result.stderr,flag
    for role in ('_KERNEL_MODE','PIONEER_DRIVER_BUILD_TAG','PIONEER_Q38_TOKEN_NATIVE_CONTEXT',
                 'PIONEER_Q38_HYBRID_FULL_NATIVE','PIONEER_Q38_HYBRID_FULL_CLIENT','PIONEER_Q38_FULL_OFFLOAD_NATIVE',
                 'PIONEER_Q38_FULL_OFFLOAD_CLIENT','PIONEER_Q38_BF16_FULL_NATIVE','PIONEER_Q38_BF16_FULL_CLIENT','PIONEER_Q38_WORK_INFERENCE',
                 'TT_PIONEER_Q38_WORK_MODE','PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_FULL_MODEL_CPU_REFERENCE'):
        result=subprocess.run([compiler,'-std=c++17',*flags,'-D'+role+'=1','-fsyntax-only','-x','c++',str(header)],capture_output=True)
        assert result.returncode and b'isolated offline inspection only' in result.stderr,role
print('PASS dense Q8 footprint role and geometry; NO DEVICE')
