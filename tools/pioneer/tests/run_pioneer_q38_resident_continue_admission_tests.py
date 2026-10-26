#!/usr/bin/env python3
"""Actual emitted loader; setup is synthetic DATA, NOT preceding execution."""
import os
import pathlib
import subprocess
import sys
import tempfile
from pioneer_q38_resident_continue_image_common import verify
assert len(sys.argv) in (2,3) and (len(sys.argv)==2 or sys.argv[2]=='--smoke')
image=pathlib.Path(sys.argv[1]);verify(image)
here=pathlib.Path(__file__).resolve().parent
with tempfile.TemporaryDirectory(prefix='pioneer-continue-admission-',dir='/private/tmp') as temporary:
    binary=pathlib.Path(temporary)/'test'
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-ffp-contract=off',
                    '-Wall','-Wextra','-Werror','-fsanitize=address,undefined',
                    str(here/'pioneer_q38_resident_continue_admission_test.cpp'),'-o',str(binary)],check=True)
    subprocess.run([str(binary),str(image),*sys.argv[2:]],check=True,
                   env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
