#!/usr/bin/env python3
"""Compare real RV32 RMS images on synthetic simulated endpoints only."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile

assert __debug__ and len(sys.argv)==2 and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
sdk=Path(sys.argv[1]);assert sdk.is_absolute() and sdk.is_dir()
here=Path(__file__).resolve().parent
with tempfile.TemporaryDirectory(prefix='pioneer-fast-rms-image-',dir='/private/tmp') as temporary:
    temp=Path(temporary);images=temp/'images';binary=temp/'test'
    subprocess.run([sys.executable,str(here.parent/'build_qwen38_fast_f32_hidden_rms_test_image.py'),str(sdk),str(images)],check=True)
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror',
                    '-ffp-contract=off','-fsanitize=address,undefined',str(here/'pioneer_q38_fast_f32_hidden_rms_image_test.cpp'),
                    '-o',str(binary)],check=True)
    subprocess.run([str(binary),str(images/'original'),str(images/'candidate')],check=True,
                   env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
