#!/usr/bin/env python3
"""Pinned instruction/packet workload, not on-device throughput or cycles."""
import ast
import hashlib
import os
import pathlib
import re
import subprocess
import sys
import tempfile
assert len(sys.argv)==5
here=pathlib.Path(__file__).resolve().parent
root,worker,qdp,qda=map(pathlib.Path,sys.argv[1:])
def pins(script):
 tree=ast.parse((here/script).read_text())
 return next(ast.literal_eval(n.value) for n in tree.body if isinstance(n,ast.Assign) and any(isinstance(t,ast.Name) and t.id=='PINS' for t in n.targets))
def pin(directory,records):
 for name,size,digest in records:
  data=(directory/name).read_bytes();assert len(data)==size and hashlib.sha256(data).hexdigest()==digest,(directory,name)
pin(root,pins('run_pioneer_q38_mixed_block_root_tests.py'))
pin(worker,pins('run_pioneer_q38_mixed_worker_tests.py'))
pin(qdp,(('code.bin',4836,'ff39e984f76fe0d0591e9a48ea1137a231ecdaf2390b14223a8a8922b8e2488a'),))
pin(qda,(('code.bin',14428,'e33cb65be5c53d77f486ee7ea2371a2ffc78638e5ed72bfa8b2fe2bf8cf19736'),))
flags=re.findall(r'-D[A-Z][A-Z0-9_]*(?:=[0-9]+)?',(here/'run_pioneer_q38_ffn_persistent_image_tests.sh').read_text())
with tempfile.TemporaryDirectory(prefix='pioneer-block-root-profile-',dir='/private/tmp') as tmp:
 binary=pathlib.Path(tmp)/'test'
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-ffp-contract=off','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',
  *flags,str(here/'pioneer_q38_mixed_block_root_profile_test.cpp'),'-o',str(binary)],check=True)
 subprocess.run([str(binary),*sys.argv[1:]],check=True,env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
