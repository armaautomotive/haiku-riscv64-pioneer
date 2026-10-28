#!/usr/bin/env python3
"""Pure aggregate counter algebra, NOT native/actual-packet authority."""
import os
import pathlib
import subprocess
import tempfile
here=pathlib.Path(__file__).resolve().parent
with tempfile.TemporaryDirectory(prefix='pioneer-prefix-counters-',dir='/private/tmp') as tmp:
 binary=pathlib.Path(tmp)/'test'
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',
  str(here/'pioneer_q38_recurrent_prefix_counters_test.cpp'),'-o',str(binary)],check=True)
 subprocess.run([str(binary)],check=True,env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
