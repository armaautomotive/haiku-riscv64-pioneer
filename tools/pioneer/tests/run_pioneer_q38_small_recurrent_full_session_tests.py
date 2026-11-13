#!/usr/bin/env python3
"""Simulate the recurrent upload-to-token-controller handoff."""
import os
from pathlib import Path
import subprocess
import tempfile

here = Path(__file__).resolve().parent
source = here / 'pioneer_q38_small_recurrent_full_session_test.cpp'
compiler = os.environ.get('CXX', 'c++')
assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
with tempfile.TemporaryDirectory(prefix='q38-small-full-session-', dir='/private/tmp') as tmp:
    binary = str(Path(tmp) / 'test')
    subprocess.run([compiler, '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror', '-Wno-unused-function',
                    '-Wno-misleading-indentation', '-fsanitize=address,undefined', str(source), '-o', binary], check=True)
    subprocess.run([binary], check=True, env={**os.environ, 'ASAN_OPTIONS': 'quarantine_size_mb=32', 'UBSAN_OPTIONS': 'halt_on_error=1'})
