#!/usr/bin/env python3
"""Actual pinned numeric image only; NEVER hardware or a full model pass."""
import ast
import hashlib
import os
import pathlib
import subprocess
import sys
import tempfile
assert len(sys.argv)==2 and pathlib.Path(sys.argv[1]).is_absolute()
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
here=pathlib.Path(__file__).resolve().parent;directory=pathlib.Path(sys.argv[1])
tree=ast.parse((here.parent/'build_qwen38_f32_hidden_rms_test_image.py').read_text())
pins=next(ast.literal_eval(n.value) for n in tree.body if isinstance(n,ast.Assign) and any(isinstance(t,ast.Name) and t.id=='PINS' for t in n.targets));assert pins
for name,size,digest in pins:
    data=(directory/name).read_bytes();assert len(data)==size and hashlib.sha256(data).hexdigest()==digest,name
with tempfile.TemporaryDirectory(prefix='pioneer-f32-hidden-rms-image-',dir='/private/tmp') as tmp:
    binary=pathlib.Path(tmp)/'test'
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-ffp-contract=off','-fsanitize=address,undefined',str(here/'pioneer_q38_f32_hidden_rms_image_test.cpp'),'-o',str(binary)],check=True)
    subprocess.run([str(binary),str(directory)],check=True,env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
