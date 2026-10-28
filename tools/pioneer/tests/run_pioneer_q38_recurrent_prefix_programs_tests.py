#!/usr/bin/env python3
"""Authenticate and compare every prefix program word. No device/driver IO."""
import ast
import hashlib
import os
import pathlib
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
with tempfile.TemporaryDirectory(prefix='pioneer-prefix-programs-',dir='/private/tmp') as tmp:
 generated=pathlib.Path(tmp)/'PioneerQ38RecurrentPrefixPrograms.h'
 subprocess.run([sys.executable,str(here.parent/'generate_qwen38_recurrent_prefix_programs.py'),str(root),str(worker),str(generated)],check=True)
 assert generated.read_bytes()==(here.parent/'PioneerQ38RecurrentPrefixPrograms.h').read_bytes()
 binary=pathlib.Path(tmp)/'test'
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',
  str(here/'pioneer_q38_recurrent_prefix_programs_test.cpp'),'-o',str(binary)],check=True)
 subprocess.run([str(binary),str(root),str(worker)],check=True,env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
