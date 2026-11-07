#!/usr/bin/env python3
"""Build/interpret fixed BFP4 GDDR images through shared L1; no device IO."""
import os
from pathlib import Path
import subprocess
import tempfile

here=Path(__file__).resolve().parent
sdk=Path('/Volumes/HaikuBuildLocal/generated.riscv64')
assert sdk.is_dir() and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
with tempfile.TemporaryDirectory(prefix='pioneer-bfp4-dram-image-',dir='/private/tmp') as tmp:
    root=Path(tmp)
    binary=root/'test'
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror',
                    '-Wno-misleading-indentation','-Wno-unused-function','-ffp-contract=off','-fsanitize=address,undefined',
                    str(here/'pioneer_bfp4_dram_band_image_test.cpp'),'-o',str(binary)],check=True)
    for width in (5120,6144,17408):
        output=root/str(width)
        subprocess.run(['python3',str(here.parent/'build_bfp4_dram_band.py'),str(sdk),str(output),str(width)],check=True)
        subprocess.run([str(binary),str(output),str(width)],check=True,
                       env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
