#!/usr/bin/env python3
"""Actual new held L1 journal with simulated IO,not native execution."""
import os
import pathlib
import subprocess
import sys
import tempfile
assert sys.argv[1:] in ([],['--faults'])
here=pathlib.Path(__file__).resolve().parent
with tempfile.TemporaryDirectory(prefix='pioneer-live-memory-',dir='/private/tmp') as temp:
 binary=pathlib.Path(temp)/'test'
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-ffp-contract=off','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',
  str(here/'pioneer_q38_mixed_live_memory_test.cpp'),'-o',str(binary)],check=True)
 subprocess.run([str(binary),*sys.argv[1:]],check=True,env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
