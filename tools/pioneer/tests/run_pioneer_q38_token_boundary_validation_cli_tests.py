#!/usr/bin/env python3
"""Actual argument/delay helpers,MOCK clocks. No model/device access."""
import os
import pathlib
import subprocess
import tempfile

assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
here=pathlib.Path(__file__).resolve().parent
with tempfile.TemporaryDirectory(prefix='pioneer-boundary75-cli-',dir='/private/tmp') as temporary:
    binary=pathlib.Path(temporary)/'test'
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror',
        '-fsanitize=address,undefined',str(here/'pioneer_q38_token_boundary_validation_cli_test.cpp'),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True,env={**os.environ,'UBSAN_OPTIONS':'halt_on_error=1'})
