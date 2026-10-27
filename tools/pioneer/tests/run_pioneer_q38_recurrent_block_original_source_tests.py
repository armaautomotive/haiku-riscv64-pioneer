#!/usr/bin/env python3
"""Independent recurrent CPU oracle/source;never native or accelerator code."""
import os
import pathlib
import subprocess
import sys
import tempfile
assert len(sys.argv)==2
here=pathlib.Path(__file__).resolve().parent
with tempfile.TemporaryDirectory(prefix='pioneer-qrb-original-source-',dir='/private/tmp') as temp:
 binary=pathlib.Path(temp)/'test'
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-ffp-contract=off',
  '-fsanitize=address,undefined',str(here/'pioneer_q38_recurrent_block_original_source_test.cpp'),
  str(here.parent/'pioneer_q38_recurrent_block_original_source.cpp'),'-o',str(binary)],check=True)
 subprocess.run([str(binary),sys.argv[1]],check=True,env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
