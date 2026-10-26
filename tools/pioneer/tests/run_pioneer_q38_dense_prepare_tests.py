#!/usr/bin/env python3
"""Pinned NEW local preparation instruction test,NO accelerator access."""
import hashlib
import os
import pathlib
import subprocess
import sys
import tempfile
assert len(sys.argv)==2
image=pathlib.Path(sys.argv[1]);here=pathlib.Path(__file__).resolve().parent
for name,size,digest in (
    ('boot.bin',4,'d0565e4ef0f59aa08ad0be91ffae61ceb85aedff5f36f346c95312b0f9407ac6'),
    ('code.bin',4836,'ff39e984f76fe0d0591e9a48ea1137a231ecdaf2390b14223a8a8922b8e2488a')):
    data=(image/name).read_bytes();assert len(data)==size and hashlib.sha256(data).hexdigest()==digest
with tempfile.TemporaryDirectory(prefix='pioneer-dense-prepare-test-',dir='/private/tmp') as temporary:
    binary=pathlib.Path(temporary)/'test'
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-ffp-contract=off','-Wall','-Wextra','-Werror',
                    '-fsanitize=address,undefined',str(here/'pioneer_q38_dense_prepare_image_test.cpp'),'-o',str(binary)],check=True)
    subprocess.run([str(binary),str(image)],check=True,
                   env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
