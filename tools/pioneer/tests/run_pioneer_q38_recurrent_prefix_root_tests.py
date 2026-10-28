#!/usr/bin/env python3
"""Actual autonomous prefix instructions. Simulated raw endpoints; no card IO."""
import ast
import hashlib
import os
import pathlib
import re
import subprocess
import sys
import tempfile
faults=len(sys.argv)==4 and sys.argv[3]=='--faults'
assert len(sys.argv)==3 or faults
here=pathlib.Path(__file__).resolve().parent;root,worker=map(pathlib.Path,sys.argv[1:3])
PINS=(('boot.bin',4,'32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca'),
 ('text.bin',56384,'f6488118d403c1e236ccf51944161ad1c95b29d6c90f8ad9625091d834982783'),
 ('conv.bin',7500,'fe4788123cd06f9b382bf1cf6305dae29c7a8f2ba51d863e835abca7c07fe892'),
 ('program.elf',79572,'ad412886874b1ce67628136e52b0755bccb0e1675ae99d2205a8b203426d8e39'))
def pin(directory,pins):
 for name,size,digest in pins:
  data=(directory/name).read_bytes();assert len(data)==size and hashlib.sha256(data).hexdigest()==digest,(directory,name)
pin(root,PINS)
tree=ast.parse((here/'run_pioneer_q38_mixed_worker_tests.py').read_text())
pin(worker,next(ast.literal_eval(n.value) for n in tree.body if isinstance(n,ast.Assign) and any(isinstance(t,ast.Name) and t.id=='PINS' for t in n.targets)))
flags=re.findall(r'-D[A-Z][A-Z0-9_]*(?:=[0-9]+)?',(here/'run_pioneer_q38_ffn_persistent_image_tests.sh').read_text())
with tempfile.TemporaryDirectory(prefix='pioneer-autonomous-prefix-',dir='/private/tmp') as tmp:
 binary=pathlib.Path(tmp)/'test'
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-ffp-contract=off',
  '-fsanitize=address,undefined',*flags,str(here/('pioneer_q38_recurrent_prefix_fault_test.cpp' if faults else 'pioneer_q38_recurrent_prefix_root_test.cpp')),'-o',str(binary)],check=True)
 subprocess.run([str(binary),str(root),str(worker)],check=True,env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
