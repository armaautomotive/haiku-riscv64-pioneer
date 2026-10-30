#!/usr/bin/env python3
"""Actual G3 opaque peer/RAW byte endpoints;no native/device invocation."""
import ast
import hashlib
import os
import pathlib
import re
import subprocess
import sys
import tempfile

assert len(sys.argv)==3 and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
old,worker=map(pathlib.Path,sys.argv[1:])
here=pathlib.Path(__file__).resolve().parent
for directory,script in ((old,here/'run_pioneer_q38_mixed_worker_tests.py'),(worker,here.parent/'build_qwen38_pushed_worker_image.py')):
    assert directory.is_absolute()
    tree=ast.parse(script.read_text())
    pins=next(ast.literal_eval(n.value) for n in tree.body if isinstance(n,ast.Assign)
              and any(isinstance(t,ast.Name) and t.id=='PINS' for t in n.targets))
    assert pins
    for name,size,sha in pins:
        raw=(directory/name).read_bytes()
        assert len(raw)==size and hashlib.sha256(raw).hexdigest()==sha,(directory,name)
tool='/Volumes/HaikuBuildLocal/generated.riscv64/cross-tools-riscv64/bin/riscv64-unknown-haiku-'
assert subprocess.check_output([tool+'nm','-n',str(worker/'program.elf')],text=True)==(worker/'program.symbols').read_text()
flags=re.findall(r'-D[A-Z][A-Z0-9_]*(?:=[0-9]+)?',(here/'run_pioneer_q38_ffn_persistent_image_tests.sh').read_text())
with tempfile.TemporaryDirectory(prefix='pioneer-native-pushed-storage-',dir='/private/tmp') as temporary:
    binary=pathlib.Path(temporary)/'test'
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-ffp-contract=off',
                    '-fsanitize=address,undefined',*flags,'-DPIONEER_Q38_TOKEN_PUSHED_PROGRAMS=1',
                    str(here/'pioneer_q38_native_pushed_storage_test.cpp'),str(here/'pioneer_q38_native_peer_test_bridge.cpp'),
                    '-o',str(binary)],check=True)
    subprocess.run([str(binary),str(old),str(worker)],check=True,
                   env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
