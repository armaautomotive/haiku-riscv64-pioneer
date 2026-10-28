#!/usr/bin/env python3
"""Authenticated RAW three-layer cache and private CPU reference. NO device IO."""
import os
import pathlib
import subprocess
import sys
import tempfile
if len(sys.argv)!=2 or any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ):raise SystemExit(2)
here=pathlib.Path(__file__).resolve().parent
with tempfile.TemporaryDirectory(prefix='pioneer-prefix-original-source-',dir='/private/tmp') as tmp:
 binary=pathlib.Path(tmp)/'test'
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-ffp-contract=off',
  '-fsanitize=address,undefined',str(here/'pioneer_q38_recurrent_prefix_original_source_test.cpp'),
  str(here.parent/'pioneer_q38_recurrent_prefix_original_source.cpp'),'-o',str(binary)],check=True)
 subprocess.run([str(binary),sys.argv[1]],check=True,env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
