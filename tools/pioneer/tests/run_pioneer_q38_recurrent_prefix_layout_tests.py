#!/usr/bin/env python3
"""Pure prefix DATA geometry and compile-time roles. No model/device IO."""
import os
import pathlib
import subprocess
import sys
import tempfile
here=pathlib.Path(__file__).resolve().parent
subprocess.run([sys.executable,str(here/'pioneer_q38_recurrent_prefix_image_gates_test.py')],check=True)
with tempfile.TemporaryDirectory(prefix='pioneer-prefix-layout-',dir='/private/tmp') as tmp:
 binary=pathlib.Path(tmp)/'test'
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-ffp-contract=off',
  '-fsanitize=address,undefined',str(here/'pioneer_q38_recurrent_prefix_layout_test.cpp'),'-o',str(binary)],check=True)
 subprocess.run([str(binary)],check=True,env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
