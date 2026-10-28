#!/usr/bin/env python3
"""Separate CPU whole-reference math/state/binding tests, NEVER device IO."""
import os
import pathlib
import subprocess
import sys
import tempfile
assert len(sys.argv) == 1 or (len(sys.argv) == 2 and sys.argv[1] == '--full-zero')
assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
here = pathlib.Path(__file__).resolve().parent
with tempfile.TemporaryDirectory(prefix='pioneer-cpu-model-reference-', dir='/private/tmp') as tmp:
    binary = pathlib.Path(tmp)/'test'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror', '-ffp-contract=off', '-fsanitize=address,undefined',
                    str(here/'pioneer_q38_cpu_model_reference_test.cpp'), str(here/'pioneer_q38_cpu_model_reference.cpp'), '-o', str(binary)], check=True)
    subprocess.run([str(binary), *sys.argv[1:]], check=True, env={**os.environ, 'ASAN_OPTIONS':'quarantine_size_mb=32', 'UBSAN_OPTIONS':'halt_on_error=1'})
