#!/usr/bin/env python3
"""Actual returning attention/FFN instructions, simulated raw endpoints only."""
import ast
import hashlib
import os
import pathlib
import re
import subprocess
import sys
import tempfile
assert len(sys.argv)==6
legacy,worker,qdp,qda,linked=map(pathlib.Path,sys.argv[1:]);here=pathlib.Path(__file__).resolve().parent
def pin(directory,records):
 for name,size,digest in records:
  data=(directory/name).read_bytes();assert len(data)==size and hashlib.sha256(data).hexdigest()==digest,(directory,name)
for directory,script in ((legacy,'run_pioneer_q38_mixed_live_block_root_tests.py'),(worker,'run_pioneer_q38_mixed_worker_tests.py'),(linked,'build_pioneer_q38_layer_return_test_link.py')):
 tree=ast.parse((here/script).read_text());pins=next(ast.literal_eval(n.value) for n in tree.body if isinstance(n,ast.Assign) and any(isinstance(t,ast.Name) and t.id=='PINS' for t in n.targets));pin(directory,pins)
pin(qdp,(('code.bin',4836,'ff39e984f76fe0d0591e9a48ea1137a231ecdaf2390b14223a8a8922b8e2488a'),))
pin(qda,(('code.bin',14428,'e33cb65be5c53d77f486ee7ea2371a2ffc78638e5ed72bfa8b2fe2bf8cf19736'),))
assert all(p.is_absolute() for p in (legacy,worker,qdp,qda,linked))
flags=re.findall(r'-D[A-Z][A-Z0-9_]*(?:=[0-9]+)?',(here/'run_pioneer_q38_ffn_persistent_image_tests.sh').read_text())
with tempfile.TemporaryDirectory(prefix='pioneer-return-dense-',dir='/private/tmp') as tmp:
 binary=pathlib.Path(tmp)/'test'
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-ffp-contract=off',
  '-fsanitize=address,undefined',*flags,str(here/'pioneer_q38_return_dense_layer_test.cpp'),'-o',str(binary)],check=True)
 subprocess.run([str(binary),*sys.argv[1:]],check=True,env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
