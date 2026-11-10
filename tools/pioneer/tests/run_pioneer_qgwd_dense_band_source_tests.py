#!/usr/bin/env python3
"""Synthetic source refusal and role checks; no real model or device."""
import os
from pathlib import Path
import subprocess
import tempfile
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
here=Path(__file__).resolve().parent
base=[os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror',
      '-ffp-contract=off','-I'+str(here/'fixtures/qgwd-dense-source')]
source=str(here/'pioneer_qgwd_dense_band_source_test.cpp')
role='-DPIONEER_NATIVE_QGWD_DENSE_BAND_SOURCE=1'
for forbidden in ('_KERNEL_MODE','PIONEER_DRIVER_BUILD_TAG','PIONEER_Q38_TOKEN_NATIVE_CONTEXT',
 'PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE','PIONEER_Q38_CPU_VALIDATION',
 'PIONEER_Q38_FULL_MODEL_CPU_REFERENCE','__FAST_MATH__'):
    r=subprocess.run([*base,role,'-D'+forbidden+'=1','-fsyntax-only',source],capture_output=True,text=True)
    assert r.returncode,forbidden
with tempfile.TemporaryDirectory(prefix='qgwd-dense-source-',dir='/private/tmp') as tmp:
    out=Path(tmp)/'test'
    subprocess.run([*base,role,'-fsanitize=address,undefined',source,
                    str(here.parent/'PioneerModelWeightCache.cpp'),'-o',str(out)],check=True)
    subprocess.run([str(out)],check=True,env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32',
                                             'UBSAN_OPTIONS':'halt_on_error=1'})
print('PASS dense-band source invalid-input/rounding and forbidden-role checks; synthetic metadata only')
