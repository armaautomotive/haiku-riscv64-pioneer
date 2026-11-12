#!/usr/bin/env python3
"""Exercise exact recurrent selection and frozen byte forwarding offline."""
import os
from pathlib import Path
import subprocess
import tempfile

here=Path(__file__).resolve().parent
root=here.parent
source=here/'pioneer_q38_recurrent_bfp8_prepare_test.cpp'
compiler=os.environ.get('CXX','c++')
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
with tempfile.TemporaryDirectory(prefix='q38-recurrent-bfp8-',dir='/private/tmp') as tmp:
    binary=str(Path(tmp)/'test')
    subprocess.run([compiler,'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-ffp-contract=off',
                    '-fsanitize=address,undefined',str(source),str(root/'PioneerModelWeightCache.cpp'),'-o',binary],check=True)
    subprocess.run([binary],check=True,env=dict(os.environ,ASAN_OPTIONS='quarantine_size_mb=32',UBSAN_OPTIONS='halt_on_error=1'))
    for forbidden in ('_KERNEL_MODE','PIONEER_DRIVER_BUILD_TAG','PIONEER_Q38_TOKEN_NATIVE_CONTEXT',
                      'PIONEER_Q38_HYBRID_BFP8_PREPARER','PIONEER_Q38_DENSE_Q8_BFP8_PREPARER',
                      'PIONEER_Q38_HYBRID_BFP8_WINDOW11','PIONEER_Q38_WORK_INFERENCE',
                      'TT_PIONEER_Q38_WORK_MODE','PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_FULL_MODEL_CPU_REFERENCE','__FAST_MATH__'):
        result=subprocess.run([compiler,'-std=c++17','-D'+forbidden+'=1','-E',str(source)],capture_output=True)
        assert result.returncode and b'Recurrent Q8 BFP8 selection is a separate CPU-only preparation role' in result.stderr,forbidden
    for flag in ('PIONEER_Q38_RECURRENT_Q8_BFP8_PREPARER','PIONEER_Q38_ALL_BFP8_CACHE'):
        result=subprocess.run([compiler,'-std=c++17','-DPIONEER_Q38_RECURRENT_Q8_BFP8_PREPARER=1',
                               '-DPIONEER_Q38_ALL_BFP8_CACHE=1','-U'+flag,'-D'+flag+'=0',
                               '-DPIONEER_Q38_RECURRENT_BFP8_TILE_TEST_ONLY=1','-fsyntax-only',
                               str(root/'pioneer_q38_recurrent_bfp8_prepare.cpp')],capture_output=True)
        assert result.returncode and b'Recurrent Q8 preparer requires its exact distinct all-BFP8 role' in result.stderr,flag
print('PASS recurrent BFP8 preparation role isolation; NO DEVICE')
