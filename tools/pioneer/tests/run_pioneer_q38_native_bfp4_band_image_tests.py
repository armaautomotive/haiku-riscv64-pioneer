#!/usr/bin/env python3
"""Compile RV32 variants and interpret them offboard; never stage to hardware."""
import os
from pathlib import Path
import subprocess
import tempfile

here=Path(__file__).resolve().parent
build=Path('/Volumes/HaikuBuildLocal/generated.riscv64')
assert build.is_dir()
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
cxx=os.environ.get('CXX','c++')
with tempfile.TemporaryDirectory(prefix='pioneer-bfp4-image-',dir='/private/tmp') as tmp:
    root=Path(tmp)
    test=root/'image-test'
    subprocess.run([cxx,'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-Wno-misleading-indentation',
                    '-fsanitize=address,undefined',str(here/'pioneer_q38_native_bfp4_band_image_test.cpp'),'-o',str(test)],check=True)
    for width in (5120,6144,17408):
        for mode in ('mmio','direct'):
            out=root/(str(width)+'-'+mode)
            command=['python3',str(here.parent/'build_native_bfp4_band.py'),str(build),str(out),str(width)]
            if mode=='direct':
                command.append('--direct')
            subprocess.run(command,check=True)
            subprocess.run([str(test),str(out),str(width),mode],check=True,
                           env={**os.environ,'UBSAN_OPTIONS':'halt_on_error=1'})
