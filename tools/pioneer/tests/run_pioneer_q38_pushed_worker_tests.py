#!/usr/bin/env python3
"""Pinned actual QGW3 sender with synthetic raw tensors/NoC;never hardware."""
import ast
import hashlib
import os
import pathlib
import re
import subprocess
import sys
import tempfile

assert len(sys.argv)==3 and not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
old, image = map(pathlib.Path, sys.argv[1:])
here = pathlib.Path(__file__).resolve().parent
for directory, script in ((old,here/'run_pioneer_q38_mixed_worker_tests.py'),
                          (image,here.parent/'build_qwen38_pushed_worker_image.py')):
    assert directory.is_absolute()
    tree=ast.parse(script.read_text())
    pins=next(ast.literal_eval(n.value) for n in tree.body if isinstance(n,ast.Assign)
              and any(isinstance(t,ast.Name) and t.id=='PINS' for t in n.targets))
    assert pins, 'independent BUILD pins must be frozen first'
    for name,size,digest in pins:
        data=(directory/name).read_bytes()
        assert len(data)==size and hashlib.sha256(data).hexdigest()==digest,(directory,name)
tool='/Volumes/HaikuBuildLocal/generated.riscv64/cross-tools-riscv64/bin/riscv64-unknown-haiku-'
assert subprocess.check_output([tool+'nm','-n',str(image/'program.elf')],text=True)==(image/'program.symbols').read_text()
flags=re.findall(r'-D[A-Z][A-Z0-9_]*(?:=[0-9]+)?',(here/'run_pioneer_q38_ffn_persistent_image_tests.sh').read_text())
with tempfile.TemporaryDirectory(prefix='pioneer-qgw3-publisher-',dir='/private/tmp') as temporary:
    binary=pathlib.Path(temporary)/'test'
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-ffp-contract=off',
                    '-fsanitize=address,undefined',*flags,str(here/'pioneer_q38_pushed_worker_test.cpp'),'-o',str(binary)],check=True)
    subprocess.run([str(binary),str(old),str(image)],check=True,
                   env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
    queued=pathlib.Path(temporary)/'queued'
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-ffp-contract=off',
                    '-fsanitize=address,undefined',*flags,str(here/'pioneer_q38_pushed_queued_job_test.cpp'),'-o',str(queued)],check=True)
    subprocess.run([str(queued),str(old),str(image)],check=True,
                   env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
    endpoint=pathlib.Path(temporary)/'endpoint'
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-ffp-contract=off',
                    '-fsanitize=address,undefined',*flags,'-DPIONEER_Q38_GLOBAL_CHAIN_PRECISE_ACTIVATION=1',
                    '-DPIONEER_Q38_GLOBAL_CHAIN_PUSHED_COMPLETION=1',
                    str(here/'pioneer_q38_pushed_endpoint_formats_test.cpp'),'-o',str(endpoint)],check=True)
    subprocess.run([str(endpoint),str(old),str(image)],check=True,
                   env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
