#!/usr/bin/env python3
"""Actual recurrent QRP1 and119 retained QMW1 workers; no device IO."""
import os
import ast
import hashlib
import pathlib
import re
import subprocess
import sys
import tempfile
assert len(sys.argv) in (5,6) and (len(sys.argv)==5 or sys.argv[5]=='--full')
here=pathlib.Path(__file__).resolve().parent
root,worker,qdp,qda=map(pathlib.Path,sys.argv[1:5])
PINS=(
 ('boot.bin',4,'ce5f73f31ecb80edc4dd267e1f46b0534a0201b837ba6d6231a5d4271df10102'),
 ('code.bin',16856,'c9fdb4ea5bf82707d78ae015ed95f5b189ef608f5c406367d3419e3b6a2a474b'),
 ('program.elf',27208,'86826199f23aedb418a23a882902476ee729b9f6c2a47e28669b5f9b48a9b147'))
def pin(directory,pins):
 for name,size,digest in pins:
  data=(directory/name).read_bytes();assert len(data)==size and hashlib.sha256(data).hexdigest()==digest,(directory,name)
pin(root,PINS)
tree=ast.parse((here/'run_pioneer_q38_mixed_worker_tests.py').read_text())
pin(worker,next(ast.literal_eval(n.value) for n in tree.body if isinstance(n,ast.Assign) and any(isinstance(t,ast.Name) and t.id=='PINS' for t in n.targets)))
pin(qdp,(('code.bin',4836,'ff39e984f76fe0d0591e9a48ea1137a231ecdaf2390b14223a8a8922b8e2488a'),))
pin(qda,(('code.bin',14428,'e33cb65be5c53d77f486ee7ea2371a2ffc78638e5ed72bfa8b2fe2bf8cf19736'),))
flags=re.findall(r'-D[A-Z][A-Z0-9_]*(?:=[0-9]+)?',(here/'run_pioneer_q38_ffn_persistent_image_tests.sh').read_text())
with tempfile.TemporaryDirectory(prefix='pioneer-mixed-recurrent-projection-test-',dir='/private/tmp') as tmp:
 binary=pathlib.Path(tmp)/'test'
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-ffp-contract=off','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',
                 *flags,str(here/'pioneer_q38_mixed_recurrent_projection_root_test.cpp'),'-o',str(binary)],check=True)
 subprocess.run([str(binary),*sys.argv[1:]],check=True,env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
