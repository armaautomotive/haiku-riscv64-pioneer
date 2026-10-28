#!/usr/bin/env python3
"""Returning-component build pins + actual worker receipt DATA; no ROOT launch."""
import ast
import hashlib
import os
import pathlib
import re
import subprocess
import sys
import tempfile
assert len(sys.argv) in (4,5)
component,old,worker=map(pathlib.Path,sys.argv[1:4]);here=pathlib.Path(__file__).resolve().parent
assert all(p.is_absolute() for p in (component,old,worker))
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
for directory,script in ((component,here.parent/'build_qwen38_global_token_components.py'),(old,here/'run_pioneer_q38_mixed_worker_tests.py'),(worker,here.parent/'build_qwen38_global_worker_image.py')):
 tree=ast.parse(script.read_text());pins=next(ast.literal_eval(n.value) for n in tree.body if isinstance(n,ast.Assign) and any(isinstance(t,ast.Name) and t.id=='PINS' for t in n.targets));assert pins
 for name,size,digest in pins:
  data=(directory/name).read_bytes();assert len(data)==size and hashlib.sha256(data).hexdigest()==digest,(directory,name)
nm='/Volumes/HaikuBuildLocal/generated.riscv64/cross-tools-riscv64/bin/riscv64-unknown-haiku-nm'
assert subprocess.check_output([nm,'-n',str(worker/'program.elf')],text=True)==(worker/'program.symbols').read_text()
flags=re.findall(r'-D[A-Z][A-Z0-9_]*(?:=[0-9]+)?',(here/'run_pioneer_q38_ffn_persistent_image_tests.sh').read_text())
with tempfile.TemporaryDirectory(prefix='pioneer-global-output-closure-',dir='/private/tmp') as tmp:
 binary=pathlib.Path(tmp)/'test'
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-ffp-contract=off','-fsanitize=address,undefined',
  *flags,str(here/'pioneer_q38_global_output_closure_test.cpp'),str(here/'pioneer_q38_global_output_original_source.cpp'),'-o',str(binary)],check=True)
 subprocess.run([str(binary),*sys.argv[2:]],check=True,
  env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
