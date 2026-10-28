#!/usr/bin/env python3
"""Authenticated original prefix, actual emitted instructions, separate oracle."""
import ast
import hashlib
import os
import pathlib
import re
import subprocess
import sys
import tempfile
assert len(sys.argv)==5
here=pathlib.Path(__file__).resolve().parent;model,embedding,root,worker=map(pathlib.Path,sys.argv[1:])
assert model.stat().st_size==18973870528
def records(path):
 tree=ast.parse(path.read_text())
 return next(ast.literal_eval(n.value) for n in tree.body if isinstance(n,ast.Assign)
  and any(isinstance(t,ast.Name) and t.id=='PINS' for t in n.targets))
def pin(directory,pins):
 for name,size,digest in pins:
  data=(directory/name).read_bytes();assert len(data)==size and hashlib.sha256(data).hexdigest()==digest,(directory,name)
pin(root,records(here/'run_pioneer_q38_recurrent_prefix_root_tests.py'))
pin(worker,records(here/'run_pioneer_q38_mixed_worker_tests.py'))
pin(embedding,(('code.bin',3856,'c754b24f767f33a00e61d2789cea3bda8ae656f6d58a2d20517311673a5aaaa1'),
 ('boot.bin',4,'d0565e4ef0f59aa08ad0be91ffae61ceb85aedff5f36f346c95312b0f9407ac6')))
flags=re.findall(r'-D[A-Z][A-Z0-9_]*(?:=[0-9]+)?',(here/'run_pioneer_q38_ffn_persistent_image_tests.sh').read_text())
common=[os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-ffp-contract=off','-fsanitize=address,undefined']
with tempfile.TemporaryDirectory(prefix='pioneer-prefix-original-instructions-',dir='/private/tmp') as tmp:
 objects=[]
 for name,selection in [('pioneer_q38_recurrent_prefix_original_instructions.cpp',flags),
  ('pioneer_q38_recurrent_prefix_original_embedding.cpp',[]),('../pioneer_q38_recurrent_prefix_original_source.cpp',[]),
  ('pioneer_q38_recurrent_prefix_original_instructions_test.cpp',[])]:
  obj=pathlib.Path(tmp)/(pathlib.Path(name).stem+'.o');subprocess.run([*common,*selection,'-c',str(here/name),'-o',str(obj)],check=True);objects.append(obj)
 binary=pathlib.Path(tmp)/'test';subprocess.run([*common,*map(str,objects),'-o',str(binary)],check=True)
 subprocess.run([str(binary),*sys.argv[1:]],check=True,env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
