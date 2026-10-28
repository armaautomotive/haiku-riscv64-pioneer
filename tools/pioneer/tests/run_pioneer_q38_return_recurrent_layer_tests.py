#!/usr/bin/env python3
"""Actual returning recurrent instructions over simulated raw endpoints only."""
import ast
import hashlib
import os
import pathlib
import re
import subprocess
import sys
import tempfile
assert len(sys.argv)==4 or len(sys.argv)==5 and sys.argv[4]=='--refusals'
legacy,worker,linked=map(pathlib.Path,sys.argv[1:4]);here=pathlib.Path(__file__).resolve().parent
for directory,script in ((legacy,'run_pioneer_q38_recurrent_prefix_root_tests.py'),(worker,'run_pioneer_q38_mixed_worker_tests.py'),(linked,'build_pioneer_q38_layer_return_test_link.py')):
 tree=ast.parse((here/script).read_text());pins=next(ast.literal_eval(n.value) for n in tree.body if isinstance(n,ast.Assign) and any(isinstance(t,ast.Name) and t.id=='PINS' for t in n.targets))
 for name,size,digest in pins:
  data=(directory/name).read_bytes();assert len(data)==size and hashlib.sha256(data).hexdigest()==digest,(directory,name)
assert all(p.is_absolute() for p in (legacy,worker,linked))
flags=re.findall(r'-D[A-Z][A-Z0-9_]*(?:=[0-9]+)?',(here/'run_pioneer_q38_ffn_persistent_image_tests.sh').read_text())
with tempfile.TemporaryDirectory(prefix='pioneer-return-recurrent-',dir='/private/tmp') as tmp:
 binary=pathlib.Path(tmp)/'test'
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-ffp-contract=off',
  '-fsanitize=address,undefined',*flags,str(here/'pioneer_q38_return_recurrent_layer_test.cpp'),'-o',str(binary)],check=True)
 subprocess.run([str(binary),*sys.argv[1:]],check=True,env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
