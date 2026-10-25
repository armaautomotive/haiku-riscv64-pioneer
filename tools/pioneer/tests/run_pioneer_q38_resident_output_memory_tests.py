#!/usr/bin/env python3
"""NEW full held-memory journal with simulated endpoints only, never card."""
import os
import pathlib
import subprocess
import sys
import tempfile

if sys.argv[1:] not in ([], ['--faults']):
    raise SystemExit(2)
here = pathlib.Path(__file__).resolve().parent
with tempfile.TemporaryDirectory(prefix='pioneer-output-memory-', dir='/private/tmp') as temporary:
    binary = pathlib.Path(temporary)/'test'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=address,undefined', str(here/'pioneer_q38_resident_output_memory_test.cpp'), '-o', str(binary)], check=True)
    subprocess.run([str(binary), *sys.argv[1:]], check=True,
                   env={**os.environ, 'ASAN_OPTIONS':'quarantine_size_mb=32', 'UBSAN_OPTIONS':'halt_on_error=1'})
