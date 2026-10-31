#!/usr/bin/env python3
"""Pinned actual preparation instructions, not a HOST arithmetic substitute."""
import ast
import hashlib
import os
import pathlib
import subprocess
import sys
import tempfile
assert len(sys.argv)==2 and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
image=pathlib.Path(sys.argv[1]);here=pathlib.Path(__file__).resolve().parent
assert image.is_absolute() and image.is_dir() and not image.is_symlink()
tree=ast.parse((here.parent/'build_qwen38_precise_beta_prepare_image.py').read_text())
pins=next(ast.literal_eval(n.value) for n in tree.body if isinstance(n,ast.Assign) and any(isinstance(t,ast.Name) and t.id=='PINS' for t in n.targets))
assert pins,'Independent emitted pins must be frozen first'
for name,size,digest in pins:
    data=(image/name).read_bytes();assert len(data)==size and hashlib.sha256(data).hexdigest()==digest
with tempfile.TemporaryDirectory(prefix='pioneer-q38-beta-prepare-test-',dir='/private/tmp') as temp:
    binary=pathlib.Path(temp)/'test'
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-ffp-contract=off',
                    '-fsanitize=address,undefined',str(here/'pioneer_q38_precise_beta_prepare_test.cpp'),'-o',str(binary)],check=True)
    subprocess.run([str(binary),str(image)],check=True,env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
