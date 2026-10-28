#!/usr/bin/env python3
"""Pinned QGW1 arithmetic/order/partial execution; not a full model launch."""
import ast
import hashlib
import os
import pathlib
import re
import subprocess
import sys
import tempfile
assert len(sys.argv) in (3,4)
old,image=map(pathlib.Path,sys.argv[1:3]);here=pathlib.Path(__file__).resolve().parent
assert old.is_absolute() and image.is_absolute() and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
for directory,script in ((old,here/'run_pioneer_q38_mixed_worker_tests.py'),(image,here.parent/'build_qwen38_global_worker_image.py')):
 tree=ast.parse(script.read_text());pins=next(ast.literal_eval(n.value) for n in tree.body if isinstance(n,ast.Assign) and any(isinstance(t,ast.Name) and t.id=='PINS' for t in n.targets))
 for name,size,digest in pins:
  data=(directory/name).read_bytes();assert len(data)==size and hashlib.sha256(data).hexdigest()==digest,(directory,name)
if len(sys.argv)==4:
 model=pathlib.Path(sys.argv[3]);assert model.is_absolute() and model.is_file()
flags=re.findall(r'-D[A-Z][A-Z0-9_]*(?:=[0-9]+)?',(here/'run_pioneer_q38_ffn_persistent_image_tests.sh').read_text())
with tempfile.TemporaryDirectory(prefix='pioneer-global-worker-arithmetic-',dir='/private/tmp') as temp:
 binary=pathlib.Path(temp)/'test'
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-ffp-contract=off',
  '-fsanitize=address,undefined',*flags,str(here/'pioneer_q38_global_worker_arithmetic_test.cpp'),
  str(here/'pioneer_q38_global_output_original_source.cpp'),'-o',str(binary)],check=True)
 subprocess.run([str(binary),*sys.argv[1:]],check=True,env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
