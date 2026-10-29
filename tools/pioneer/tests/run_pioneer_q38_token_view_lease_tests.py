#!/usr/bin/env python3
"""VM-handle ownership DATA tests; no hardware or native readiness grant."""
import os
import pathlib
import subprocess
import tempfile
assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
here = pathlib.Path(__file__).resolve().parent
with tempfile.TemporaryDirectory(prefix='pioneer-token-views-', dir='/private/tmp') as temporary:
    binary = pathlib.Path(temporary)/'test'
    subprocess.run([os.environ.get('CXX','c++'), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=address,undefined', str(here/'pioneer_q38_token_view_lease_test.cpp'), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
