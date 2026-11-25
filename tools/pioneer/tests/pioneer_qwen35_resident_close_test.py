#!/usr/bin/env python3
"""Exercise the resident close transition with simulated device operations."""
import os
from pathlib import Path
import subprocess
import tempfile

source = Path(__file__).with_suffix('.cpp')
with tempfile.TemporaryDirectory() as directory:
    for name, extra in (('ordinary', []), ('paired', ['-DPIONEER_QWEN35_PAIRED_RECURRENCE'])):
        binary = Path(directory) / name
        subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra',
                        '-Werror', '-fsanitize=address,undefined', *extra, str(source), '-o', str(binary)],
                       check=True)
        env = dict(os.environ, ASAN_OPTIONS='quarantine_size_mb=32', UBSAN_OPTIONS='halt_on_error=1')
        subprocess.run([str(binary)], check=True, env=env)
