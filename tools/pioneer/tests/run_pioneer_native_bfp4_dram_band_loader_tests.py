#!/usr/bin/env python3
"""Test fixed BFP4 packed-GDDR load/compute descriptors; no hardware IO."""
import os
from pathlib import Path
import subprocess
import tempfile

here=Path(__file__).resolve().parent
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
cxx=os.environ.get('CXX','c++')
with tempfile.TemporaryDirectory(prefix='pioneer-bfp4-dram-',dir='/private/tmp') as tmp:
    binary=Path(tmp)/'test'
    subprocess.run([cxx,'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-Wno-misleading-indentation',
                    '-fsanitize=address,undefined',str(here/'pioneer_native_bfp4_dram_band_loader_test.cpp'),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True,env={**os.environ,'UBSAN_OPTIONS':'halt_on_error=1'})
    source='#include "PioneerNativeBfp4DramBand.h"\n'
    for macro in ('_KERNEL_MODE','PIONEER_Q38_WORK_INFERENCE','PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_FULL_MODEL_CPU_REFERENCE'):
        for value in ('0','1'):
            result=subprocess.run([cxx,'-std=c++17','-fsyntax-only','-x','c++','-I',str(here.parent),'-D'+macro+'='+value,'-'],
                                  input=source,text=True,capture_output=True)
            assert result.returncode and 'Unqualified BFP4 GDDR band' in result.stderr,(macro,value,result.stderr)
    print('PASS BFP4 GDDR loader role refusals: eight defined-zero/one kernel/Work/reference cases; NO device')
