#!/usr/bin/env python3
"""Execute the pinned RV32 capture writer in a synthetic machine, not a card."""
import hashlib
import os
from pathlib import Path
import subprocess
import sys
import tempfile

if len(sys.argv) != 2 or any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ):
    raise SystemExit('one absolute image directory required')
image = Path(sys.argv[1])
if not image.is_absolute() or not image.is_dir() or image.is_symlink():
    raise SystemExit('invalid image directory')
expected = {'program.elf': '5a8c25655145ced3ebe95d0322f6a6fe7b6b69c3a284d70fdd50312fe644efcd',
            'text.bin': 'b6c62b4688d8111ec866d07e83b063c612d48d18ed763425f628eeab9796a02f',
            'conv.bin': 'bd0de31c6db2d054d0023108029d2e4f07d0fe9c0f9ecd4063bd27f24c1e2ec6',
            'program.symbols': 'd75e37aa1972d2228473b5eb6cd1fdf48ba634852c124b40dee1fab1cc50656a'}
for name, digest in expected.items():
    path = image / name
    if path.is_symlink() or hashlib.sha256(path.read_bytes()).hexdigest() != digest:
        raise SystemExit('image identity refused: ' + name)
here = Path(__file__).resolve().parent
with tempfile.TemporaryDirectory(prefix='qgw8-dense-input-image-', dir='/private/tmp') as tmp:
    binary = Path(tmp) / 'test'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O2', '-Wall', '-Wextra', '-Werror',
                    '-Wno-unused-function', '-Wno-unused-variable', '-fsanitize=address,undefined',
                    str(here / 'pioneer_q38_qgw8_dense_input_image_test.cpp'), '-o', str(binary)], check=True)
    subprocess.run([str(binary), str(image)], check=True, env=dict(os.environ, UBSAN_OPTIONS='halt_on_error=1'))
