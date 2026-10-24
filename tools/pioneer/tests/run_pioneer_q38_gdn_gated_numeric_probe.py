#!/usr/bin/env python3
"""Original-source CPU/emitted-local diagnostic;NO device or native context."""
import hashlib
import os
import pathlib
import struct
import subprocess
import sys
import tempfile

if len(sys.argv) != 3 or any(k.startswith('PIONEER_') and v != '0' for k, v in os.environ.items()):
    raise SystemExit(2)
model, image = map(pathlib.Path, sys.argv[1:])
assert model.is_file() and model.stat().st_size == 18973870528
assert hashlib.sha256((image/'code.bin').read_bytes()).hexdigest() == '7ec76ef20a0a453ae24adbda74623b5f95a400ff0f5b15e17e3788f079beb2b8'
assert (image/'boot.bin').read_bytes() == struct.pack('<I', 0x0005e06f)
here = pathlib.Path(__file__).resolve().parent
with tempfile.TemporaryDirectory(prefix='pioneer-qgt-numeric-probe-', dir='/private/tmp') as temporary:
    root = pathlib.Path(temporary)
    common = [os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
              '-ffp-contract=off', '-fsanitize=address,undefined']
    bridge = root/'local-emitted-observer.o'
    subprocess.run([*common, '-c', str(here/'pioneer_q38_gdn_gated_norm_bridge.cpp'), '-o', str(bridge)], check=True)
    binary = root/'probe'
    subprocess.run([*common, str(here/'pioneer_q38_gdn_gated_numeric_probe.cpp'), str(bridge), '-o', str(binary)], check=True)
    subprocess.run([str(binary), str(model), str(image)], check=True,
                   env={**os.environ, 'ASAN_OPTIONS': 'quarantine_size_mb=32', 'UBSAN_OPTIONS': 'halt_on_error=1'})
