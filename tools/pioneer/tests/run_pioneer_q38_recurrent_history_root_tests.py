#!/usr/bin/env python3
"""History-staging ROOT/SAME119/three token-zero prefix layers. No device IO."""
import ast
import hashlib
import os
import pathlib
import re
import subprocess
import sys
import tempfile
assert len(sys.argv)==3
here=pathlib.Path(__file__).resolve().parent
root,worker=map(pathlib.Path,sys.argv[1:3])
def pin(directory,pins):
 for name,size,digest in pins:
  data=(directory/name).read_bytes();assert len(data)==size and hashlib.sha256(data).hexdigest()==digest,(directory,name)
pin(root,(('boot.bin',4,'32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca'),
 ('text.bin',53084,'6ad600096827d4f60b4cc73b7901a6999bd5c75c5311b32e2763fe700e6b7652'),
 ('conv.bin',7500,'16ff564dc406a72a918f7c67c40f6815a7430f6d02161a0be0b26ec56cde7f65'),
 ('program.elf',75184,'a1ae94663803c6cfc760ef242275658008cda99b5611e193a11a9958a3375339')))
tree=ast.parse((here/'run_pioneer_q38_mixed_worker_tests.py').read_text())
pin(worker,next(ast.literal_eval(n.value) for n in tree.body if isinstance(n,ast.Assign) and any(isinstance(t,ast.Name) and t.id=='PINS' for t in n.targets)))
flags=re.findall(r'-D[A-Z][A-Z0-9_]*(?:=[0-9]+)?',(here/'run_pioneer_q38_ffn_persistent_image_tests.sh').read_text())
subprocess.run([sys.executable,str(here/'pioneer_q38_recurrent_history_image_gates_test.py')],check=True)
with tempfile.TemporaryDirectory(prefix='pioneer-recurrent-history-',dir='/private/tmp') as tmp:
 binary=pathlib.Path(tmp)/'test'
 source='pioneer_q38_recurrent_history_root_test.cpp'
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-ffp-contract=off',
  '-fsanitize=address,undefined',*flags,str(here/source),'-o',str(binary)],check=True)
 subprocess.run([str(binary),str(root),str(worker)],check=True,env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
