#!/usr/bin/env python3
"""Execute the pinned combined worker on a mocked NoC and Tensix fabric."""
import hashlib
import os
from pathlib import Path
import subprocess
import sys
import tempfile

if len(sys.argv) != 4 or any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ):
    raise SystemExit('usage: ABS_WORKER_A ABS_WORKER_B ABS_BATCH_LEAF')
first, second, leaf = map(Path, sys.argv[1:])
if not all(p.is_absolute() and p.is_dir() for p in (first, second, leaf)):
    raise SystemExit('absolute existing directories required')
def pin(path, digest):
    assert hashlib.sha256(path.read_bytes()).hexdigest() == digest, path
for folder in (first, second):
    for name, digest in (
        ('program.elf', 'd3614997705d473f6a2a0c0f23dabcbf1418f587bf7686c3862bcd55c81aaa42'),
        ('globalworker.bin', '7e7282754f9d50027893166429a972325124d461fcb3c9ce527dcc3d44af562f'),
        ('cachedfpu.bin', '2212c2bd6d304df6c4ba7e97ee8e0db2683557f851c366545a312c03f3bc9973')):
        pin(folder/name, digest)
    assert 'b9042703' in (folder/'program.disassembly').read_text()
pin(leaf/'boot.bin', 'd47a1c29c265663f5bd4bd49aaf28248b7345d7feb5a4f770caa27315deaa4dd')
pin(leaf/'text.bin', '77d87adaf6ab3f9c34fc9f1c50c627d6174676b700f24bca34c8a28f0bcd1dd0')
names = ('PIONEER_Q38_QGWE_BATCH_DENSE_WORKER', 'PIONEER_Q38_QGW8_BFP8_BATCH_WORKER',
         'PIONEER_Q38_QGWD_SRCA_DENSE_Q8_WORKER', 'PIONEER_Q38_DENSE_Q8_MATRIX_WORKER_INSPECTION',
         'PIONEER_Q38_DENSE_BF16_SRCA_BASE_CANDIDATE', 'PIONEER_Q38_HEAD_MATRIX_WORKER_INSPECTION',
         'PIONEER_Q38_HYBRID_WORKER_IMAGE', 'PIONEER_Q38_BF16_WORKER_IMAGE',
         'PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION', 'PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT',
         'PIONEER_Q38_BFP8_BATCH_INPUT_EXPERIMENT')
with tempfile.TemporaryDirectory(prefix='qgwe-worker-test-', dir='/private/tmp') as tmp:
    binary = Path(tmp)/'test'
    source = Path(__file__).resolve().parent/'pioneer_q38_hybrid_head_worker_image_test.cpp'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra',
                    '-Werror', '-ffp-contract=off', '-fsanitize=address,undefined',
                    *['-D'+name+'=1' for name in names], str(source), '-o', str(binary)], check=True)
    for folder in (first, second):
        result = subprocess.run([str(binary), str(folder), str(leaf)], capture_output=True, text=True,
                                env={**os.environ, 'ASAN_OPTIONS':'quarantine_size_mb=32',
                                     'UBSAN_OPTIONS':'halt_on_error=1'}, check=True)
        assert not result.stderr and 'PASS QGWE' in result.stdout, result.stdout + result.stderr
        print(result.stdout, end='')
print('PASS two pinned workers on mocked transport; NOT full-model, hardware, quality or speed')
