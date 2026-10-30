#!/usr/bin/env python3
"""Pinned ROOT bootstrap with normal/EOS TEST session data; no original model."""
import ast
import hashlib
import os
import pathlib
import re
import subprocess
import sys
import tempfile

assert len(sys.argv)==4 and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
image,old,worker=map(pathlib.Path,sys.argv[1:]);here=pathlib.Path(__file__).resolve().parent
for directory,script,key in ((image,here.parent/'build_qwen38_token_root.py','PINS_CANCEL_F32_LOCAL_SOURCE'),
                             (old,here/'run_pioneer_q38_mixed_worker_tests.py','PINS'),
                             (worker,here.parent/'build_qwen38_global_worker_image.py','PINS')):
    assert directory.is_absolute()
    tree=ast.parse(script.read_text())
    pins=next(ast.literal_eval(n.value) for n in tree.body if isinstance(n,ast.Assign) and any(isinstance(t,ast.Name) and t.id==key for t in n.targets))
    assert pins
    for name,size,digest in pins:
        data=(directory/name).read_bytes();assert len(data)==size and hashlib.sha256(data).hexdigest()==digest,(directory,name)
tool='/Volumes/HaikuBuildLocal/generated.riscv64/cross-tools-riscv64/bin/riscv64-unknown-haiku-'
assert subprocess.check_output([tool+'nm','-n',str(image/'program.elf')],text=True)==(image/'program.symbols').read_text()
flags=re.findall(r'-D[A-Z][A-Z0-9_]*(?:=[0-9]+)?',(here/'run_pioneer_q38_ffn_persistent_image_tests.sh').read_text())
with tempfile.TemporaryDirectory(prefix='pioneer-eos-startup-',dir='/private/tmp') as tmp:
    binary=pathlib.Path(tmp)/'test'
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-ffp-contract=off',
                    '-fsanitize=address,undefined',*flags,'-DPIONEER_Q38_TEST_CANCELLABLE_ROOT=1',str(here/'pioneer_q38_eos_startup_test.cpp'),'-o',str(binary)],check=True)
    subprocess.run([str(binary),*sys.argv[1:]],check=True,env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
