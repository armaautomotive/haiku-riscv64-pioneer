#!/usr/bin/env python3
"""Default offline raw byte batching equivalence, not an image/model test."""
import os
import pathlib
import re
import subprocess
import tempfile
here = pathlib.Path(__file__).resolve().parent
assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
flags = re.findall(r'-D[A-Z][A-Z0-9_]*(?:=[0-9]+)?', (here/'run_pioneer_q38_ffn_persistent_image_tests.sh').read_text())
with tempfile.TemporaryDirectory(prefix='pioneer-cohort-raw-batch-', dir='/private/tmp') as tmp:
    binary = pathlib.Path(tmp)/'test'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror', '-ffp-contract=off', '-fsanitize=address,undefined',
                    *flags, str(here/'pioneer_q38_cohort_raw_batch_test.cpp'), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True, env={**os.environ, 'ASAN_OPTIONS':'quarantine_size_mb=32', 'UBSAN_OPTIONS':'halt_on_error=1'})
