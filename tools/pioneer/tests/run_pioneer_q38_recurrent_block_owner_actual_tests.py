#!/usr/bin/env python3
"""Production distinct owner and actual full QRB1/QMW1 instructions; no device IO."""
import ast
import hashlib
import os
import pathlib
import re
import subprocess
import sys
import tempfile
assert len(sys.argv) in (3,4) and (len(sys.argv)==3 or sys.argv[3] in ('--prepare-faults','--end-faults'))
here=pathlib.Path(__file__).resolve().parent
root,worker=map(pathlib.Path,sys.argv[1:3])
def pin(directory,pins):
 for name,size,digest in pins:
  data=(directory/name).read_bytes();assert len(data)==size and hashlib.sha256(data).hexdigest()==digest,(directory,name)
pin(root,(('boot.bin',4,'32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca'),
 ('text.bin',52596,'6da8623de3970219ca10fa092adf0d4c08e7275f92f6eec436bb99c6f333da17'),
 ('conv.bin',7500,'16ff564dc406a72a918f7c67c40f6815a7430f6d02161a0be0b26ec56cde7f65'),
 ('program.elf',75184,'dfaee3da2e38471333b9bfef3d031ef06cffbe0b6ea29df5adae8391df662f2f')))
tree=ast.parse((here/'run_pioneer_q38_mixed_worker_tests.py').read_text())
pin(worker,next(ast.literal_eval(n.value) for n in tree.body if isinstance(n,ast.Assign) and any(isinstance(t,ast.Name) and t.id=='PINS' for t in n.targets)))
flags=re.findall(r'-D[A-Z][A-Z0-9_]*(?:=[0-9]+)?',(here/'run_pioneer_q38_ffn_persistent_image_tests.sh').read_text())
with tempfile.TemporaryDirectory(prefix='pioneer-qrb-owner-actual-',dir='/private/tmp') as tmp:
 binary=pathlib.Path(tmp)/'test'
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-ffp-contract=off',
  '-fsanitize=address,undefined',*flags,str(here/('pioneer_q38_recurrent_block_owner_end_fault_test.cpp' if sys.argv[-1]=='--end-faults' else
   'pioneer_q38_recurrent_block_owner_fault_test.cpp' if len(sys.argv)==4 else 'pioneer_q38_recurrent_block_owner_actual_test.cpp')),'-o',str(binary)],check=True)
 subprocess.run([str(binary),str(root),str(worker)],check=True,env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
