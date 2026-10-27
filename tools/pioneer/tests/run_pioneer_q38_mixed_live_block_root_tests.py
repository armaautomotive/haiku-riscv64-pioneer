#!/usr/bin/env python3
"""Pinned fresh119/device-input/compute/STOP lifecycle,not native timing."""
import ast
import hashlib
import os
import pathlib
import re
import subprocess
import sys
import tempfile
assert len(sys.argv)==5 or len(sys.argv)==6 and sys.argv[-1] in ('--canonical','--frontend','--owner')
canonical=len(sys.argv)==6
frontend=canonical and sys.argv[-1]=='--frontend'
owner=canonical and sys.argv[-1]=='--owner'
here=pathlib.Path(__file__).resolve().parent
root,worker,qdp,qda=map(pathlib.Path,sys.argv[1:5])
PINS=(
 ('boot.bin',4,'32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca'),
 ('code.bin',50320,'50009c46fcdefb427dc1b31b68cc9b2403b4ba8e06b377ebd66a88bf7ce254c8'),
 ('program.elf',62528,'4c9392c44cb470c648d49f8c7209f9cbb2a3e84fe879f355fa75622b551ffe47'))
def pin(directory,records):
 for name,size,digest in records:
  data=(directory/name).read_bytes();assert len(data)==size and hashlib.sha256(data).hexdigest()==digest,(directory,name)
pin(root,PINS)
tree=ast.parse((here/'run_pioneer_q38_mixed_worker_tests.py').read_text())
pin(worker,next(ast.literal_eval(n.value) for n in tree.body if isinstance(n,ast.Assign) and any(isinstance(t,ast.Name) and t.id=='PINS' for t in n.targets)))
pin(qdp,(('code.bin',4836,'ff39e984f76fe0d0591e9a48ea1137a231ecdaf2390b14223a8a8922b8e2488a'),))
pin(qda,(('code.bin',14428,'e33cb65be5c53d77f486ee7ea2371a2ffc78638e5ed72bfa8b2fe2bf8cf19736'),))
flags=re.findall(r'-D[A-Z][A-Z0-9_]*(?:=[0-9]+)?',(here/'run_pioneer_q38_ffn_persistent_image_tests.sh').read_text())
if frontend:flags+=['-DPIONEER_Q38_TEST_FRONTEND_ENDPOINTS=1']
with tempfile.TemporaryDirectory(prefix='pioneer-live-block-test-',dir='/private/tmp') as tmp:
 binary=pathlib.Path(tmp)/'test'
 source='pioneer_q38_mixed_live_owner_actual_test.cpp' if owner else 'pioneer_q38_mixed_live_canonical_joined_test.cpp' if canonical else 'pioneer_q38_mixed_live_block_root_test.cpp'
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-ffp-contract=off','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',
  *flags,str(here/source),'-o',str(binary)],check=True)
 subprocess.run([str(binary),*sys.argv[1:5]],check=True,env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
