#!/usr/bin/env python3
"""Test the isolated immutable peer-program DATA callback."""
import os
from pathlib import Path
import subprocess
import tempfile

here = Path(__file__).resolve().parent
assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
with tempfile.TemporaryDirectory(prefix='q38-peer-program-word-', dir='/private/tmp') as tmp:
    binary = str(Path(tmp) / 'test')
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-Wall', '-Wextra', '-Werror',
                    '-Wno-misleading-indentation', '-fsanitize=address,undefined',
                    str(here/'pioneer_q38_native_peer_program_word_test.cpp'), '-o', binary], check=True)
    subprocess.run([binary], check=True)
