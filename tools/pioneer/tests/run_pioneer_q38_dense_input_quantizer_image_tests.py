#!/usr/bin/env python3
"""Execute standalone pinned RV32 quantizer with CPU DATA; never open a card."""
import hashlib
import os
from pathlib import Path
import subprocess
import sys
import tempfile

if len(sys.argv)!=2 or any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ):
    raise SystemExit('absolute standalone image directory required')
image=Path(sys.argv[1]);here=Path(__file__).resolve().parent
reference=here.parents[2]/'generated.pioneer/qualification-inputs/qgw8-dense-stage-physical-20261109/cpu-attention-input-v1.bin'
items=((image/'code.bin','9538d7e293cde56af2a7d9a0bbb13503b3088c0b72ebb7f4a70481c26a37db56'),
       (image/'boot.bin','d0565e4ef0f59aa08ad0be91ffae61ceb85aedff5f36f346c95312b0f9407ac6'),
       (reference,'972b0747f8b03d422a7c5315ccd0fd15bf227231b1c316d5e7026c53eaf0ebf0'))
if not image.is_absolute() or image.is_symlink() or not image.is_dir():raise SystemExit('invalid image directory')
for path,digest in items:
    if path.is_symlink() or hashlib.sha256(path.read_bytes()).hexdigest()!=digest:raise SystemExit('input identity refused: '+str(path))
with tempfile.TemporaryDirectory(prefix='qgw8-quantizer-image-',dir='/private/tmp') as tmp:
    binary=Path(tmp)/'test'
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-ffp-contract=off','-Wall','-Wextra','-Werror',
                    '-fsanitize=address,undefined',str(here/'pioneer_q38_dense_input_quantizer_image_test.cpp'),'-o',str(binary)],check=True)
    subprocess.run([str(binary),str(image),str(reference)],check=True,
                   env=dict(os.environ,ASAN_OPTIONS='quarantine_size_mb=32',UBSAN_OPTIONS='halt_on_error=1'))
