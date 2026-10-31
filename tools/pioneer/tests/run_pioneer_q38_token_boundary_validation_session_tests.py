#!/usr/bin/env python3
"""Actual observed-boundary USER session over MOCK source/OS/driver replies."""
import os
import pathlib
import subprocess
import tempfile

assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
here=pathlib.Path(__file__).resolve().parent
with tempfile.TemporaryDirectory(prefix='pioneer-boundary75-session-',dir='/private/tmp') as temporary:
    binary=pathlib.Path(temporary)/'test'
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror',
                    '-ffp-contract=off','-fsanitize=address,undefined',
                    str(here/'pioneer_q38_token_boundary_validation_session_test.cpp'),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True,env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
