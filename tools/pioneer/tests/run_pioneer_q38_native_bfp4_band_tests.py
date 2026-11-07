#!/usr/bin/env python3
"""Host-only instruction-producer checks and unsafe-role refusals."""
import os
from pathlib import Path
import subprocess
import tempfile

here=Path(__file__).resolve().parent
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
cxx=os.environ.get('CXX','c++')
with tempfile.TemporaryDirectory(prefix='pioneer-bfp4-band-',dir='/private/tmp') as tmp:
 binary=Path(tmp)/'test'
 subprocess.run([cxx,'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',
                 str(here/'pioneer_q38_native_bfp4_band_test.cpp'),'-o',str(binary)],check=True)
 subprocess.run([str(binary)],check=True,env={**os.environ,'UBSAN_OPTIONS':'halt_on_error=1'})
 numeric=Path(tmp)/'numeric'
 subprocess.run([cxx,'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-Wno-misleading-indentation',
                 '-fsanitize=address,undefined',str(here/'pioneer_q38_native_bfp4_band_numeric_test.cpp'),'-o',str(numeric)],check=True)
 subprocess.run([str(numeric)],check=True,env={**os.environ,'UBSAN_OPTIONS':'halt_on_error=1'})
 source='#include "PioneerQ38NativeBfp4Band.h"\n'
 for macro in ('_KERNEL_MODE','PIONEER_Q38_WORK_INFERENCE','PIONEER_Q38_CPU_VALIDATION',
               'PIONEER_Q38_FULL_MODEL_CPU_REFERENCE','__FAST_MATH__'):
  for value in ('0','1'):
   result=subprocess.run([cxx,'-std=c++17','-fsyntax-only','-x','c++','-I',str(here.parent),'-D'+macro+'='+value,'-'],
                         input=source,text=True,capture_output=True)
   assert result.returncode and 'unqualified shader prototype' in result.stderr,(macro,value,result.stderr)
 print('PASS BFP4 producer role refusals: 10 defined-zero/one driver, Work, reference and fast-math cases; NO_DEVICE')
