#!/usr/bin/env python3
"""Test the isolated fixed-size dense-Q8 geometry; never open hardware."""
import os
from pathlib import Path
import subprocess
import tempfile

assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
here=Path(__file__).resolve().parent
source=here/'pioneer_q38_dense_q8_kernel_layout_test.cpp'
compiler=os.environ.get('CXX','c++')
with tempfile.TemporaryDirectory(prefix='q38-q8-fixed-',dir='/private/tmp') as tmp:
    binary=Path(tmp)/'test'
    subprocess.run([compiler,'-std=c++17','-O2','-g','-Wall','-Wextra','-Werror',
                    '-Wno-misleading-indentation','-Wno-unused-function','-fsanitize=address,undefined',str(source),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True,env=dict(os.environ,UBSAN_OPTIONS='halt_on_error=1'))
    for role in ('_KERNEL_MODE','PIONEER_DRIVER_BUILD_TAG','PIONEER_Q38_TOKEN_NATIVE_CONTEXT',
                 'PIONEER_Q38_FULL_OFFLOAD_NATIVE','PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE'):
        result=subprocess.run([compiler,'-std=c++17','-D'+role,'-E',str(source)],capture_output=True)
        assert result.returncode and b'Dense Q8 fixed layout is isolated inspection only' in result.stderr,role
print('PASS dense-Q8 fixed-layout role exclusions; NO DEVICE')
