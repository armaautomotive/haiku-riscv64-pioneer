#!/usr/bin/env python3
"""Independent finite counter geometry;not instruction/native qualification."""
import os
import pathlib
import subprocess
import sys
import tempfile
assert len(sys.argv)==1
here=pathlib.Path(__file__).resolve().parent
with tempfile.TemporaryDirectory(prefix='pioneer-recurrent-counters-',dir='/private/tmp') as tmp:
 binary=pathlib.Path(tmp)/'test'
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror',
  '-fsanitize=address,undefined',str(here/'pioneer_q38_recurrent_block_counters_test.cpp'),'-o',str(binary)],check=True)
 subprocess.run([str(binary)],check=True,env={**os.environ,'UBSAN_OPTIONS':'halt_on_error=1'})
