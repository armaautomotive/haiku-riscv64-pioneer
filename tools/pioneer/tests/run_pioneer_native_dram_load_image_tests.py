#!/usr/bin/env python3
"""Compile/execute actual fixed LOAD-ONLY82 image in strict RV32 VM;NO GPU."""
import os
import pathlib
import subprocess
import sys
import tempfile

if not __debug__ or len(sys.argv) != 2:
    raise SystemExit('Use non-optimized Python HAIKU_BUILD')
here = pathlib.Path(__file__).resolve().parent
folder = pathlib.Path(tempfile.mkdtemp(prefix='pioneer-dram-load-only-image.'))
subprocess.run([sys.executable, str(here.parent / 'build_native_dram_load.py'), sys.argv[1], str(folder / 'image')], check=True)
subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                '-fsanitize=address,undefined', str(here / 'pioneer_native_dram_load_image_test.cpp'), '-o', str(folder / 'test')], check=True)
subprocess.run([str(folder / 'test'), str(folder / 'image')], check=True,
               env=dict(os.environ, ASAN_OPTIONS='quarantine_size_mb=32', UBSAN_OPTIONS='halt_on_error=1'))
print('OFFBOARD_LOAD_ONLY82_IMAGE_EVIDENCE:', folder)
