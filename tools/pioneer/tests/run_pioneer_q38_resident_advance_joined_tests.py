#!/usr/bin/env python3
"""Actual emitted advance/parent/119peer programs. NO hardware or CPU products."""
import os
import pathlib
import re
import subprocess
import sys
import tempfile
from pioneer_q38_resident_advance_image_common import verify

assert len(sys.argv) in (2,3) and (len(sys.argv)==2 or sys.argv[2] in ('--faults','--component'))
image=pathlib.Path(sys.argv[1]);peer=verify(image)
here=pathlib.Path(__file__).resolve().parent
flags=re.findall(r'-D[A-Z][A-Z0-9_]*(?:=[0-9]+)?',(here/'run_pioneer_q38_ffn_persistent_image_tests.sh').read_text())
with tempfile.TemporaryDirectory(prefix='pioneer-resident-advance-joined-',dir='/private/tmp') as temporary:
    binary=pathlib.Path(temporary)/'test'
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-ffp-contract=off',
                    '-Wall','-Wextra','-Werror','-fsanitize=address,undefined',*flags,
                    str(here/'pioneer_q38_resident_advance_joined_test.cpp'),'-o',str(binary)],check=True)
    subprocess.run([str(binary),str(image),str(peer),*sys.argv[2:]],check=True,
                   env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
