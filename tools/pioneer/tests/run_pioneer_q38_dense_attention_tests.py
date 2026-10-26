#!/usr/bin/env python3
"""NEW wide SFPU+readonly-prefix program,not device/native/model timing."""
import hashlib
import os
import pathlib
import subprocess
import sys
import tempfile
assert len(sys.argv) in (2,3) and (len(sys.argv)==2 or sys.argv[2]=='--full')
image=pathlib.Path(sys.argv[1]);here=pathlib.Path(__file__).resolve().parent
for name,size,digest in (
    ('boot.bin',4,'d0565e4ef0f59aa08ad0be91ffae61ceb85aedff5f36f346c95312b0f9407ac6'),
    ('code.bin',14428,'e33cb65be5c53d77f486ee7ea2371a2ffc78638e5ed72bfa8b2fe2bf8cf19736')):
    data=(image/name).read_bytes();assert len(data)==size and hashlib.sha256(data).hexdigest()==digest
with tempfile.TemporaryDirectory(prefix='pioneer-dense-attention-test-',dir='/private/tmp') as temporary:
    binary=pathlib.Path(temporary)/'test'
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-ffp-contract=off','-Wall','-Wextra','-Werror',
                    '-fsanitize=address,undefined',str(here/'pioneer_q38_dense_attention_image_test.cpp'),'-o',str(binary)],check=True)
    subprocess.run([str(binary),str(image),*sys.argv[2:]],check=True,
                   env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
