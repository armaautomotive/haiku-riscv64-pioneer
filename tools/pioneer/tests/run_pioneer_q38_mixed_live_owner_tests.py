#!/usr/bin/env python3
"""Host owner lifetime test: explicitly synthetic endpoints, not device/TPS."""
import os
import pathlib
import subprocess
import sys
import tempfile
assert sys.argv[1:] in ([], ['--faults'], ['--preconditions'])
here=pathlib.Path(__file__).resolve().parent
with tempfile.TemporaryDirectory(prefix='pioneer-live-owner-',dir='/private/tmp') as temp:
 binary=pathlib.Path(temp)/'test'
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-ffp-contract=off','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',
  str(here/'pioneer_q38_mixed_live_owner_test.cpp'),'-o',str(binary)],check=True)
 subprocess.run([str(binary),*sys.argv[1:]],check=True,env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
