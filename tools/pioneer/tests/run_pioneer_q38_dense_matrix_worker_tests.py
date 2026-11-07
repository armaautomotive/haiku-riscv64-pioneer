#!/usr/bin/env python3
"""Observe actual isolated QDM2 instructions; never connect to hardware."""
import hashlib
import os
import pathlib
import subprocess
import sys
import tempfile

if len(sys.argv) != 3:
    raise SystemExit('usage: run_pioneer_q38_dense_matrix_worker_tests.py QDM2_DIR QDA_DIR')
matrix, original = map(pathlib.Path, sys.argv[1:])
for directory, name, size, digest in (
    (matrix, 'boot.bin', 4, '32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca'),
    (matrix, 'code.bin', 14252, '0896f4edc570ab4f338550353448ae0d587b89895ffdf35684edd55aa9902f12'),
    (original, 'boot.bin', 4, 'd0565e4ef0f59aa08ad0be91ffae61ceb85aedff5f36f346c95312b0f9407ac6'),
    (original, 'code.bin', 14428, 'e33cb65be5c53d77f486ee7ea2371a2ffc78638e5ed72bfa8b2fe2bf8cf19736'),
):
    data = (directory/name).read_bytes()
    assert len(data) == size and hashlib.sha256(data).hexdigest() == digest
here = pathlib.Path(__file__).resolve().parent
with tempfile.TemporaryDirectory(prefix='pioneer-dense-matrix-worker-', dir='/private/tmp') as temporary:
    binary = pathlib.Path(temporary)/'test'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-ffp-contract=off',
                    '-Wall', '-Wextra', '-Werror', '-fsanitize=address,undefined',
                    str(here/'pioneer_q38_dense_matrix_worker_test.cpp'), '-o', str(binary)], check=True)
    subprocess.run([str(binary), str(matrix), str(original)], check=True,
                   env={**os.environ, 'ASAN_OPTIONS':'quarantine_size_mb=32', 'UBSAN_OPTIONS':'halt_on_error=1'})
