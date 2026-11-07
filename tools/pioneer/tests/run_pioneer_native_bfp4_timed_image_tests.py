#!/usr/bin/env python3
"""Build and interpret timed BFP4 RV32 images; no card access."""
import os
from pathlib import Path
import subprocess
import tempfile

here=Path(__file__).resolve().parent
sdk=Path('/Volumes/HaikuBuildLocal/generated.riscv64')
assert sdk.is_dir() and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
with tempfile.TemporaryDirectory(prefix='pioneer-bfp4-timed-',dir='/private/tmp') as temp:
    root=Path(temp)
    test=root/'test'
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror',
                    '-Wno-misleading-indentation','-fsanitize=address,undefined',
                    str(here/'pioneer_native_bfp4_timed_image_test.cpp'),'-o',str(test)],check=True)
    for width in (5120,6144,17408):
        for mode in ('mmio','direct'):
            out=root/(str(width)+'-'+mode)
            cmd=['python3',str(here.parent/'build_native_bfp4_band.py'),str(sdk),str(out),str(width),'--timed']
            if mode=='direct':
                cmd.append('--direct')
            subprocess.run(cmd,check=True)
            subprocess.run([str(test),str(out),str(width),mode],check=True,env={**os.environ,'UBSAN_OPTIONS':'halt_on_error=1'})
