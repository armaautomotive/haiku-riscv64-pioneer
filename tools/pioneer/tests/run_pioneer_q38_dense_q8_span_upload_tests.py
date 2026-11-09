#!/usr/bin/env python3
"""Exercise the isolated dense-Q8 span adapter without hardware access."""
import os
from pathlib import Path
import subprocess
import tempfile

assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
here=Path(__file__).resolve().parent
source=here/'pioneer_q38_dense_q8_span_upload_test.cpp'
compiler=os.environ.get('CXX','c++')
with tempfile.TemporaryDirectory(prefix='q38-q8-span-',dir='/private/tmp') as tmp:
    binary=Path(tmp)/'test'
    subprocess.run([compiler,'-std=c++17','-O2','-g','-Wall','-Wextra','-Werror',
                    '-Wno-misleading-indentation','-fsanitize=address,undefined',str(source),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True,env=dict(os.environ,ASAN_OPTIONS='quarantine_size_mb=32',UBSAN_OPTIONS='halt_on_error=1'))
    prepared=Path(tmp)/'prepared'
    subprocess.run([compiler,'-std=c++17','-O2','-g','-Wall','-Wextra','-Werror',
                    '-Wno-misleading-indentation','-Wno-unused-function','-fsanitize=address,undefined',
                    str(here/'pioneer_q38_dense_q8_prepared_upload_test.cpp'),
                    str(here.parent/'PioneerModelWeightCache.cpp'),'-o',str(prepared)],check=True)
    subprocess.run([str(prepared)],check=True,env=dict(os.environ,ASAN_OPTIONS='quarantine_size_mb=32',UBSAN_OPTIONS='halt_on_error=1'))
    for role in ('_KERNEL_MODE','PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE',
                 'PIONEER_Q38_TOKEN_NATIVE_CONTEXT','PIONEER_DRIVER_BUILD_TAG','PIONEER_Q38_FULL_OFFLOAD_CLIENT'):
        result=subprocess.run([compiler,'-std=c++17','-D'+role,'-E',str(source)],capture_output=True)
        assert result.returncode and b'Hybrid span upload is a separate experimental diagnostic' in result.stderr,role
print('PASS isolated upload role exclusions; NO DEVICE')
