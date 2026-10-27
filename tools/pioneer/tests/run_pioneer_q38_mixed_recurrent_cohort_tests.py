#!/usr/bin/env python3
"""Execute NEW cohort ROOT and unchanged119 QMW1 workers,not hardware."""
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
root,worker=map(pathlib.Path,sys.argv[1:])
def pin(directory,pins):
 for name,size,digest in pins:
  data=(directory/name).read_bytes();assert len(data)==size and hashlib.sha256(data).hexdigest()==digest,(directory,name)
pin(root,(('boot.bin',4,'32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca'),
 ('text.bin',23448,'6e31ae597aa9b3491e34daa389d6b19eb724a9db18efc37ee00f43699f36b8ef'),
 ('conv.bin',7500,'9674113d7c69cf7290b03e4bef133a8aa30cb1e6193dc7411b7941bae603cc78'),
 ('program.elf',43876,'b177dfbee14d830a56d286e1eb0eb9c22143d63d542d08e0ca7b869c9a13777e')))
tree=ast.parse((here/'run_pioneer_q38_mixed_worker_tests.py').read_text())
pin(worker,next(ast.literal_eval(n.value) for n in tree.body if isinstance(n,ast.Assign) and any(isinstance(t,ast.Name) and t.id=='PINS' for t in n.targets)))
flags=re.findall(r'-D[A-Z][A-Z0-9_]*(?:=[0-9]+)?',(here/'run_pioneer_q38_ffn_persistent_image_tests.sh').read_text())
with tempfile.TemporaryDirectory(prefix='pioneer-mixed-recurrent-cohort-',dir='/private/tmp') as tmp:
 binary=pathlib.Path(tmp)/'test'
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-ffp-contract=off',
  '-fsanitize=address,undefined',*flags,str(here/'pioneer_q38_mixed_recurrent_cohort_test.cpp'),'-o',str(binary)],check=True)
 subprocess.run([str(binary),str(root),str(worker)],check=True,env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
