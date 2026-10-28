#!/usr/bin/env python3
"""Distinct prefix owner + actual emitted3layer instructions. NO native/card IO."""
import ast
import hashlib
import os
import pathlib
import re
import subprocess
import sys
import tempfile
assert len(sys.argv)==3
here=pathlib.Path(__file__).resolve().parent;root,worker=map(pathlib.Path,sys.argv[1:])
def records(path):
 tree=ast.parse(path.read_text())
 return next(ast.literal_eval(n.value) for n in tree.body if isinstance(n,ast.Assign) and any(isinstance(t,ast.Name) and t.id=='PINS' for t in n.targets))
for directory,pins in ((root,records(here/'run_pioneer_q38_recurrent_prefix_root_tests.py')),(worker,records(here/'run_pioneer_q38_mixed_worker_tests.py'))):
 for name,size,digest in pins:
  data=(directory/name).read_bytes();assert len(data)==size and hashlib.sha256(data).hexdigest()==digest,(directory,name)
flags=re.findall(r'-D[A-Z][A-Z0-9_]*(?:=[0-9]+)?',(here/'run_pioneer_q38_ffn_persistent_image_tests.sh').read_text())
with tempfile.TemporaryDirectory(prefix='pioneer-prefix-owner-actual-',dir='/private/tmp') as tmp:
 binary=pathlib.Path(tmp)/'test'
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-ffp-contract=off','-fsanitize=address,undefined',
  *flags,str(here/'pioneer_q38_recurrent_prefix_owner_actual_test.cpp'),'-o',str(binary)],check=True)
 subprocess.run([str(binary),str(root),str(worker)],check=True,env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
