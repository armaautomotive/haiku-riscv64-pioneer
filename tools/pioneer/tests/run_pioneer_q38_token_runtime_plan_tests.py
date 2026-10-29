#!/usr/bin/env python3
"""Exact complete startup write recipe and frozen raw RoPE DATA,NO device IO."""
import hashlib
import os
import pathlib
import re
import struct
import subprocess
import tempfile
assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
here = pathlib.Path(__file__).resolve().parent
header = (here.parent/'PioneerQ38TokenRope.h').read_text()
words = [int(w, 16) for w in re.findall(r'0x([0-9a-f]{8})U', header)]
assert len(words) == 524288
assert hashlib.sha256(struct.pack('<524288I', *words)).hexdigest() == '23abf02bc6bfede090d3434921b2fd9e84b2e9c46e5ae8cb033658740a6305a9'
with tempfile.TemporaryDirectory(prefix='pioneer-token-runtime-plan-', dir='/private/tmp') as tmp:
    binary = pathlib.Path(tmp)/'test'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-ffp-contract=off', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=address,undefined', str(here/'pioneer_q38_token_runtime_plan_test.cpp'), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True, env={**os.environ, 'ASAN_OPTIONS':'quarantine_size_mb=32', 'UBSAN_OPTIONS':'halt_on_error=1'})
