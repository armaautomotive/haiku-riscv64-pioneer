#!/usr/bin/env python3
"""Actual frozen candidate worker, synthetic endpoints; NO native/card IO."""
import ast
import hashlib
import os
import pathlib
import re
import subprocess
import sys
import tempfile
assert len(sys.argv)==3 and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
old,image=map(pathlib.Path,sys.argv[1:]);here=pathlib.Path(__file__).resolve().parent
for directory,script in ((old,here/'run_pioneer_q38_mixed_worker_tests.py'),(image,here.parent/'build_qwen38_precise_beta_worker_image.py')):
    assert directory.is_absolute() and directory.is_dir() and not directory.is_symlink()
    tree=ast.parse(script.read_text())
    pins=next(ast.literal_eval(n.value) for n in tree.body if isinstance(n,ast.Assign) and any(isinstance(t,ast.Name) and t.id=='PINS' for t in n.targets))
    assert pins
    for name,size,digest in pins:
        data=(directory/name).read_bytes();assert len(data)==size and hashlib.sha256(data).hexdigest()==digest
flags=re.findall(r'-D[A-Z][A-Z0-9_]*(?:=[0-9]+)?',(here/'run_pioneer_q38_ffn_persistent_image_tests.sh').read_text())
with tempfile.TemporaryDirectory(prefix='pioneer-q38-beta-worker-',dir='/private/tmp') as temporary:
    binary=pathlib.Path(temporary)/'test'
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-ffp-contract=off','-Wall','-Wextra','-Werror',
                    '-fsanitize=address,undefined',*flags,str(here/'pioneer_q38_precise_beta_worker_test.cpp'),'-o',str(binary)],check=True)
    subprocess.run([str(binary),str(old),str(image)],check=True,env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
