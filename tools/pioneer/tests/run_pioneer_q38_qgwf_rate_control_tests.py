#!/usr/bin/env python3
"""Compile the actual control adapter with mock endpoints; no driver/device."""
import os
from pathlib import Path
import subprocess
import tempfile

here=Path(__file__).resolve().parent
source=(here.parents[2]/'src/add-ons/kernel/drivers/misc/PioneerQ38TokenNativeControl.inc').read_text()
start=source.index('template<class Result> static void Q38TokenControlDetail')
end=source.index('#ifdef PIONEER_Q38_TOKEN_FRONTIER_NATIVE_CONTEXT',start)
assert source[start:end].count('Q38TokenNativeControl(')==1
with tempfile.TemporaryDirectory(prefix='qgwf-rate-control-',dir='/private/tmp') as name:
    path=Path(name);(path/'actual_qgwf_rate_control.inc').write_text(source[start:end])
    binary=path/'test'
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror',
                    '-Wno-unused-function','-fsanitize=address,undefined','-I'+str(path),
                    str(here/'pioneer_q38_qgwf_rate_control_test.cpp'),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True,env={**os.environ,'UBSAN_OPTIONS':'halt_on_error=1'})
