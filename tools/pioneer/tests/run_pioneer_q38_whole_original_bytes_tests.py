#!/usr/bin/env python3
"""Whole authenticated GGUF RAW source qualification, NEVER device IO."""
import os
import pathlib
import subprocess
import sys
import tempfile
assert len(sys.argv)==2 and pathlib.Path(sys.argv[1]).is_absolute()
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
here=pathlib.Path(__file__).resolve().parent
with tempfile.TemporaryDirectory(prefix='pioneer-whole-original-bytes-',dir='/private/tmp') as tmp:
    binary=pathlib.Path(tmp)/'test'
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-ffp-contract=off','-fsanitize=address,undefined',
                    str(here/'pioneer_q38_whole_original_bytes_test.cpp'),str(here/'pioneer_q38_whole_original_bytes.cpp'),'-o',str(binary)],check=True)
    subprocess.run([str(binary),sys.argv[1]],check=True,env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
