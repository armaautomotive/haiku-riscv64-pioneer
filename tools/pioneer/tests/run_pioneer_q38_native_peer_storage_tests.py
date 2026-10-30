#!/usr/bin/env python3
"""Virgin full peer engine/shared byte storage component; NEVER device IO."""
import ast
import hashlib
import os
import pathlib
import re
import subprocess
import sys
import tempfile

assert (len(sys.argv)==3 or len(sys.argv)==4 and sys.argv[3]=='--precise') and not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
precise=len(sys.argv)==4
old, worker = map(pathlib.Path, sys.argv[1:3])
assert old.is_absolute() and worker.is_absolute()
here = pathlib.Path(__file__).resolve().parent
for directory, script in ((old, here/'run_pioneer_q38_mixed_worker_tests.py'), (worker, here/'run_pioneer_q38_precise_worker_tests.py' if precise else here.parent/'build_qwen38_global_worker_image.py')):
    tree = ast.parse(script.read_text())
    pins = next(ast.literal_eval(n.value) for n in tree.body if isinstance(n, ast.Assign) and any(isinstance(t, ast.Name) and t.id == 'PINS' for t in n.targets))
    for name, size, sha in pins:
        data = (directory/name).read_bytes()
        assert len(data) == size and hashlib.sha256(data).hexdigest() == sha
flags = re.findall(r'-D[A-Z][A-Z0-9_]*(?:=[0-9]+)?', (here/'run_pioneer_q38_ffn_persistent_image_tests.sh').read_text())
if precise:
    flags+=['-DPIONEER_Q38_TOKEN_PRECISE_PROGRAMS=1'] # DATA only,no native/BUILD selector
with tempfile.TemporaryDirectory(prefix='pioneer-native-peer-storage-', dir='/private/tmp') as directory:
    binary = pathlib.Path(directory)/'test'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror', '-ffp-contract=off',
                    '-fsanitize=address,undefined', *flags, str(here/'pioneer_q38_native_peer_storage_test.cpp'),
                    str(here/'pioneer_q38_native_peer_test_bridge.cpp'), '-o', str(binary)], check=True)
    subprocess.run([str(binary), str(old), str(worker)], check=True,
                   env={**os.environ, 'ASAN_OPTIONS':'quarantine_size_mb=32', 'UBSAN_OPTIONS':'halt_on_error=1'})
