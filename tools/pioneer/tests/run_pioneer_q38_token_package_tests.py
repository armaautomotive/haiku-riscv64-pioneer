#!/usr/bin/env python3
"""Persistent package/session/output-job DATA only."""
import os
import pathlib
import subprocess
import tempfile
here=pathlib.Path(__file__).resolve().parent
assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
with tempfile.TemporaryDirectory(prefix='pioneer-token-package-',dir='/private/tmp') as tmp:
    binary=pathlib.Path(tmp)/'test'
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',
                    str(here/'pioneer_q38_token_package_test.cpp'),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True,env={**os.environ,'UBSAN_OPTIONS':'halt_on_error=1'})
