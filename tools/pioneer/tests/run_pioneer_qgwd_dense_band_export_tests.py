#!/usr/bin/env python3
"""Compile-only role and CLI checks; no model, cache or device access."""
import os
from pathlib import Path
import subprocess
import tempfile

assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
here = Path(__file__).resolve().parent
source = here.parent/'pioneer_native_model_band_export.cpp'
compiler = os.environ.get('CXX','c++')
base = [compiler,'-std=c++17','-O2','-Wall','-Wextra','-Werror','-ffp-contract=off']
for role in ('_KERNEL_MODE','PIONEER_DRIVER_BUILD_TAG','PIONEER_Q38_TOKEN_NATIVE_CONTEXT',
             'PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE',
             'PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_FULL_MODEL_CPU_REFERENCE'):
    result=subprocess.run([*base,'-DPIONEER_NATIVE_QGWD_DENSE_BAND_EXPORT=1','-D'+role+'=1',
                           '-fsyntax-only',str(source)],capture_output=True,text=True)
    assert result.returncode,role
result=subprocess.run([*base,'-DPIONEER_NATIVE_QGWD_DENSE_BAND_EXPORT=0','-fsyntax-only',str(source)],capture_output=True,text=True)
assert result.returncode and 'private CPU-only preparation' in result.stderr
with tempfile.TemporaryDirectory(prefix='qgwd-real-band-role-',dir='/private/tmp') as directory:
    binary=Path(directory)/'export'
    subprocess.run([*base,'-DPIONEER_NATIVE_QGWD_DENSE_BAND_EXPORT=1',str(source),
                    str(here.parent/'PioneerModelWeightCache.cpp'),'-o',str(binary)],check=True)
    result=subprocess.run([str(binary)],capture_output=True)
    assert result.returncode==2 and not list(Path(directory).glob('PioneerNativeBfpModelBand*.h'))
print('PASS dense Q8 exporter role exclusions and no-argument refusal; no model/cache/device access')
