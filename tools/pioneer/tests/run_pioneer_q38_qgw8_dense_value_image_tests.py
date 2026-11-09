#!/usr/bin/env python3
"""Execute pinned linked value-capture instructions, no card."""
import hashlib
import os
from pathlib import Path
import subprocess
import sys
import tempfile
if len(sys.argv)!=2 or any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ):
    raise SystemExit('one absolute image directory required')
image=Path(sys.argv[1])
if not image.is_absolute() or not image.is_dir() or image.is_symlink():raise SystemExit('image path refused')
expected={'program.elf':'01da0aa95c61878ebae266921511eaa1f83df99e9bc4ee250f72fc8ac98851d7',
          'text.bin':'575a6007f62fbfdc6c030cb7bca074561d594ff3f4eacb0041beca63e1f6fb67',
          'conv.bin':'1ed7eadadf03d5dfed3470454b1910158428bb8ce2837e077b99c900c5989c2e',
          'program.symbols':'a2776719ae30c9b6574ce20f7b1448552ea7bed41e7bb692923c5b9141671d86'}
for name,digest in expected.items():
    path=image/name
    if path.is_symlink() or hashlib.sha256(path.read_bytes()).hexdigest()!=digest:raise SystemExit('image identity refused: '+name)
here=Path(__file__).resolve().parent
with tempfile.TemporaryDirectory(prefix='qgw8-dense-value-image-',dir='/private/tmp') as tmp:
    binary=Path(tmp)/'test'
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O2','-Wall','-Wextra','-Werror',
                    '-Wno-unused-function','-Wno-unused-variable','-fsanitize=address,undefined',
                    str(here/'pioneer_q38_qgw8_dense_value_image_test.cpp'),'-o',str(binary)],check=True)
    subprocess.run([str(binary),str(image)],check=True,env=dict(os.environ,UBSAN_OPTIONS='halt_on_error=1'))
