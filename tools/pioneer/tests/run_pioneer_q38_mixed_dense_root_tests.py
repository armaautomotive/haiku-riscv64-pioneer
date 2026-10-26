#!/usr/bin/env python3
"""Actual retained ROOT/all119 mixed workers/four QDP, never hardware IO."""
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
for directory,name,size,digest in (
 (root,'boot.bin',4,'49beea42a964dc73343fca928d59be0eb6a6d28e4c1354b47894f3f60ef4a31a'),
 (root,'code.bin',17488,'53e9e2d8dfceb756ae6dd6de2b72996b01b188eff13d4741bd2a6da8581fa06d'),
 (root,'program.elf',27600,'f09227e3972bf02dad64cd9ea318d4371ca33c56646de7a490885c6e0b1e7e77'),
 (qdp,'code.bin',4836,'ff39e984f76fe0d0591e9a48ea1137a231ecdaf2390b14223a8a8922b8e2488a'),
 (qda,'code.bin',14428,'e33cb65be5c53d77f486ee7ea2371a2ffc78638e5ed72bfa8b2fe2bf8cf19736')):
 data=(directory/name).read_bytes();assert len(data)==size and hashlib.sha256(data).hexdigest()==digest,name
# Reuse the actual frozen QMW1 qualification pins, without running its CLI.
tree=ast.parse((here/'run_pioneer_q38_mixed_worker_tests.py').read_text())
pins=next(ast.literal_eval(n.value) for n in tree.body if isinstance(n,ast.Assign) and any(isinstance(t,ast.Name) and t.id=='PINS' for t in n.targets))
for name,size,digest in pins:
 data=(worker/name).read_bytes();assert len(data)==size and hashlib.sha256(data).hexdigest()==digest,name
flags=re.findall(r'-D[A-Z][A-Z0-9_]*(?:=[0-9]+)?',(here/'run_pioneer_q38_ffn_persistent_image_tests.sh').read_text())
with tempfile.TemporaryDirectory(prefix='pioneer-mixed-dense-root-test-',dir='/private/tmp') as tmp:
 binary=pathlib.Path(tmp)/'test'
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-ffp-contract=off','-Wall','-Wextra','-Werror',
                 '-fsanitize=address,undefined',*flags,str(here/'pioneer_q38_mixed_dense_root_test.cpp'),'-o',str(binary)],check=True)
 subprocess.run([str(binary),*sys.argv[1:]],check=True,env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
