#!/usr/bin/env python3
"""Compute an independent CPU-private original-model trace, never device IO."""
import hashlib
import os
import pathlib
import subprocess
import sys
import tempfile
assert len(sys.argv) == 3
model, output = map(pathlib.Path, sys.argv[1:])
assert model.is_absolute() and output.is_absolute() and not output.exists() and not output.is_symlink()
assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
here = pathlib.Path(__file__).resolve().parent
output.mkdir(mode=0o700)
with tempfile.TemporaryDirectory(prefix='pioneer-cpu-original-reference-', dir='/private/tmp') as tmp:
    binary = pathlib.Path(tmp)/'test'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror', '-ffp-contract=off', '-fsanitize=address,undefined',
                    str(here/'pioneer_q38_cpu_model_original.cpp'), str(here/'pioneer_q38_cpu_model_reference.cpp'), str(here/'pioneer_q38_whole_original_bytes.cpp'), '-o', str(binary)], check=True)
    subprocess.run([str(binary), str(model), str(output)], check=True, env={**os.environ, 'ASAN_OPTIONS':'quarantine_size_mb=32', 'UBSAN_OPTIONS':'halt_on_error=1'})
    for position in range(3):
        path = output/f'token-{position}.bin'
        data = path.read_bytes()
        assert len(data) == (16+65*5120+248320+1)*4 and data[:8] == bytes.fromhex('5146523101000000') and data[-4:] == bytes.fromhex('51444e45')
        print('CPU-PRIVATE numerical trace', position, len(data), hashlib.sha256(data).hexdigest(), flush=True)
