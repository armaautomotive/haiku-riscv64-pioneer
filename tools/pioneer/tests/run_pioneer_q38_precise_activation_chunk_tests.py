#!/usr/bin/env python3
"""Bounded QAC1 host and ACTUAL emitted-instruction tests; no device IO."""
import hashlib
import os
import pathlib
import subprocess
import sys
import tempfile

if len(sys.argv) != 2 or any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ):
    raise SystemExit(2)
here = pathlib.Path(__file__).resolve().parent
image = pathlib.Path(sys.argv[1])
assert image.is_absolute() and image.is_dir() and not image.is_symlink()
pins = (('boot.bin', 4, 'd0565e4ef0f59aa08ad0be91ffae61ceb85aedff5f36f346c95312b0f9407ac6'),
        ('code.bin', 6676, '68eb14b3871294d290b12dcc9ccab9e9ce0ead86d67f23d4f61612a4d2da4934'),
        ('program.elf', 16184, '17e6e84e39a25b6dd95c2a827869d21a2f824f08524bc95f4f658f3cc780211f'))
for name, size, digest in pins:
    path = image/name
    assert path.is_file() and not path.is_symlink()
    data = path.read_bytes()
    assert len(data) == size and hashlib.sha256(data).hexdigest() == digest, name
cxx = os.environ.get('CXX', 'c++')
common = [cxx, '-std=c++17', '-O1', '-g', '-ffp-contract=off', '-Wall', '-Wextra', '-Werror',
          '-fsanitize=address,undefined']
env = {**os.environ, 'ASAN_OPTIONS': 'quarantine_size_mb=32', 'UBSAN_OPTIONS': 'halt_on_error=1'}
with tempfile.TemporaryDirectory(prefix='pioneer-q38-precise-chunk-', dir='/private/tmp') as tmp:
    base = pathlib.Path(tmp)
    for name in ('pioneer_q38_precise_activation_chunk_test', 'pioneer_q38_precise_activation_chunk_image_test'):
        binary = base/name
        subprocess.run([*common, str(here/(name+'.cpp')), '-o', str(binary)], check=True)
        subprocess.run([str(binary), *([str(image)] if name.endswith('_image_test') else [])], env=env, check=True)
print('PASS QAC1 bounded host+emitted tests;NO device access or native/Work selection', flush=True)
