#!/usr/bin/env python3
"""Test the pinned QGW7/QDM2 three-product PV inspection image; no device."""
import hashlib
import os
from pathlib import Path
import subprocess
import sys
import tempfile

if len(sys.argv) != 3:
    raise SystemExit('usage: ... QGW7_THREE_PRODUCT_DIR BFP4_LEAF_DIR')
image, leaf = map(Path, sys.argv[1:])
assert all(path.is_absolute() and path.is_dir() for path in (image, leaf))
for folder, entries in (
    (image, (
        ('boot.bin', 4, 'a12af9d63593a1f882750a147dd7a511ac476c5c7a55526bd4d51c685173991c'),
        ('globalworker.bin', 10756, 'd8080e9504c7eb00b43d7379dc1a38642b1fe5783f80b15fd0fefdc7b382621f'),
        ('bf16producer.bin', 7360, '8b3e477d51eeca467e155b72234bed728c7109947f78cfb4361ce6dbc7afb2c3'),
        ('cachedfpu.bin', 12668, 'bcc704d361936935e26c57b138693accc5a22207ab5e45019d78baec1385abc9'),
        ('dense.bin', 14460, '8719756bbe7c3b3b99d67889235a2031c40c36a539e09d70469facb1598f543b'),
        ('text.bin', 5984, '2548ed0d4fdb68ce48325cd6f8581bff6bdc5b34a87f3ba217e36e4a37ed6e5c'),
        ('affine.bin', 5360, '47c52fee6ccfb0a8847fe17a9d43d4016e7cfc6c68cc1427673f6869cd287b78'),
        ('ffn.bin', 16456, 'c1dc115dadb20b1ee1d18fb04ce61f15a1d1644a523ccfec8fea70dbd5164e45'),
        ('service.bin', 9888, '3b3cdbd88d049b3f0c72016d8eccfad7eab3fd6861a5353a50f26d7d4b46795f'),
        ('gdn.bin', 35468, '08cf9599fe94ce055f9b3136f8663093813cd6538633bc06c4a8d006556672b0'),
        ('program.elf', 156812, 'd2183e7bb86e4ad871202ac8c84a575025e0ff0078f98558949fe5654db40f93'),
    )),
    (leaf, (
        ('boot.bin', 4, 'd47a1c29c265663f5bd4bd49aaf28248b7345d7feb5a4f770caa27315deaa4dd'),
        ('text.bin', 6928, '04d43a4e5ad5b766fea07d0d3f9f48168557abda4772826d804f643db644faaf'),
    )),
):
    for name, size, digest in entries:
        data = (folder/name).read_bytes()
        assert len(data) == size and hashlib.sha256(data).hexdigest() == digest, (folder, name)

here = Path(__file__).resolve().parent
with tempfile.TemporaryDirectory(prefix='pioneer-qgw7-3pv-', dir='/private/tmp') as temporary:
    binary = Path(temporary)/'transition'
    subprocess.run([
        os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
        '-ffp-contract=off', '-fsanitize=address,undefined',
        '-DPIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION=1',
        '-DPIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT=1',
        '-DPIONEER_Q38_TEST_THREE_PRODUCT_PV=1',
        str(here/'pioneer_q38_hybrid_matrix_attention_transition_test.cpp'), '-o', str(binary),
    ], check=True)
    subprocess.run([str(binary), str(image), str(leaf)], check=True,
                   env={**os.environ, 'ASAN_OPTIONS':'quarantine_size_mb=32',
                        'UBSAN_OPTIONS':'halt_on_error=1'})
print('PASS pinned QGW7/QDM2 three-product PV shared-state transition; synthetic only, NOT Work/native/model quality/silicon/TPS')
