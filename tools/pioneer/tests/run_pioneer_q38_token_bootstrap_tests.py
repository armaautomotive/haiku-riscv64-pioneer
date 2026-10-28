#!/usr/bin/env python3
"""Pinned actual persistent ROOT bootstrap/first-stage instructions, NO hardware."""
import ast
import hashlib
import os
import pathlib
import re
import subprocess
import sys
import tempfile
assert len(sys.argv)==4
image,old,worker=map(pathlib.Path,sys.argv[1:]);here=pathlib.Path(__file__).resolve().parent
assert all(p.is_absolute() for p in (image,old,worker))
assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
for directory,script in ((image,here.parent/'build_qwen38_token_root.py'),(old,here/'run_pioneer_q38_mixed_worker_tests.py'),(worker,here.parent/'build_qwen38_global_worker_image.py')):
    tree=ast.parse(script.read_text())
    pins=next(ast.literal_eval(n.value) for n in tree.body if isinstance(n,ast.Assign) and any(isinstance(t,ast.Name) and t.id=='PINS' for t in n.targets))
    assert pins
    for name,size,digest in pins:
        data=(directory/name).read_bytes()
        assert len(data)==size and hashlib.sha256(data).hexdigest()==digest,(directory,name)
nm='/Volumes/HaikuBuildLocal/generated.riscv64/cross-tools-riscv64/bin/riscv64-unknown-haiku-nm'
for directory in (image,worker):
    assert subprocess.check_output([nm,'-n',str(directory/'program.elf')],text=True)==(directory/'program.symbols').read_text(),'symbol metadata must derive from literal-pinned ELF'
flags=re.findall(r'-D[A-Z][A-Z0-9_]*(?:=[0-9]+)?',(here/'run_pioneer_q38_ffn_persistent_image_tests.sh').read_text())
with tempfile.TemporaryDirectory(prefix='pioneer-token-bootstrap-',dir='/private/tmp') as tmp:
    binary=pathlib.Path(tmp)/'test'
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-ffp-contract=off','-fsanitize=address,undefined',
                    *flags,str(here/'pioneer_q38_token_bootstrap_test.cpp'),'-o',str(binary)],check=True)
    subprocess.run([str(binary),*sys.argv[1:]],check=True,env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
