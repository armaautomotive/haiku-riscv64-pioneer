#!/usr/bin/env python3
"""Actual installed peer boot smoke,not a synthetic/native ready grant."""
import os
import pathlib
import re
import subprocess
import sys
import tempfile

assert len(sys.argv) == 3
here = pathlib.Path(__file__).resolve().parent
flags = sorted(set(re.findall(r'-D[A-Z][A-Z0-9_]*(?:=[0-9]+)?', (here/'run_pioneer_q38_residual_rms_ffn_cooperative_tests.sh').read_text())))
with tempfile.TemporaryDirectory(prefix='pioneer-ffn-actual-boot-') as temporary:
    binary = pathlib.Path(temporary)/'test'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-ffp-contract=off', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=address,undefined', *flags, '-DPIONEER_Q38_GDN_GATED_NATIVE=1', '-DPIONEER_Q38_GDN_RECURRENT_NATIVE=1',
                    str(here/'pioneer_q38_ffn_execution_boot_test.cpp'), str(here/'pioneer_q38_ffn_residual_execution_machine.cpp'),
                    '-o', str(binary)], check=True)
    subprocess.run([str(binary), *sys.argv[1:]], check=True,
                   env={**os.environ, 'ASAN_OPTIONS': 'quarantine_size_mb=32', 'UBSAN_OPTIONS': 'halt_on_error=1'})
