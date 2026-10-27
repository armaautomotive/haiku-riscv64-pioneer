#!/usr/bin/env python3
"""Raw TEST norm-byte publication failures;not native/model correctness."""
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
for path,name,digest in ((root,'code.bin','668a72ed92b42044d6fa8263bba8c63ff19ce31b1533b67e4a4d9799b9a2047e'),
 (root,'boot.bin','32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca'),
 (qdp,'code.bin','ff39e984f76fe0d0591e9a48ea1137a231ecdaf2390b14223a8a8922b8e2488a'),
 (qda,'code.bin','e33cb65be5c53d77f486ee7ea2371a2ffc78638e5ed72bfa8b2fe2bf8cf19736')):
 assert hashlib.sha256((path/name).read_bytes()).hexdigest()==digest
tree=ast.parse((here/'run_pioneer_q38_mixed_worker_tests.py').read_text())
pins=next(ast.literal_eval(n.value) for n in tree.body if isinstance(n,ast.Assign) and any(isinstance(t,ast.Name) and t.id=='PINS' for t in n.targets))
for name,size,digest in pins:
 data=(worker/name).read_bytes();assert len(data)==size and hashlib.sha256(data).hexdigest()==digest
flags=re.findall(r'-D[A-Z][A-Z0-9_]*(?:=[0-9]+)?',(here/'run_pioneer_q38_ffn_persistent_image_tests.sh').read_text())
flags+=['-DPIONEER_Q38_MIXED_LIVE_SIGMOID_PROGRAMS=1','-DPIONEER_Q38_MIXED_LIVE_VALIDATION_READBACK=1','-DPIONEER_Q38_MIXED_LIVE_ORIGINAL_NATIVE_TEST=1']
common=[os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-ffp-contract=off','-Wall','-Wextra','-Werror','-fsanitize=address,undefined']
with tempfile.TemporaryDirectory(prefix='pioneer-original-raw-stage-',dir='/private/tmp') as build:
 base=pathlib.Path(build);obj=base/'transport.o';binary=base/'test'
 subprocess.run([*common,*flags,'-c',str(here/'pioneer_q38_mixed_live_native_transport.cpp'),'-o',str(obj)],check=True)
 subprocess.run([*common,str(here/'pioneer_q38_mixed_live_original_raw_stage_test.cpp'),str(obj),'-o',str(binary)],check=True)
 subprocess.run([str(binary),*map(str,(root,worker,qdp,qda))],check=True,env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
