#!/usr/bin/env python3
"""Actual partition workers/actual QDA producer; synthetic CONTROL, not speed."""
import hashlib
import os
import pathlib
import subprocess
import sys
import tempfile
assert len(sys.argv) in (3,4) and (len(sys.argv)==3 or sys.argv[3]=='--full')
image,original=map(pathlib.Path,sys.argv[1:3]);here=pathlib.Path(__file__).resolve().parent
for directory,name,size,digest in (
 (image,'boot.bin',4,'32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca'),
 (image,'code.bin',10804,'ecc4bd29fa2517a61737ad67e59cfce942b6363f96370774648a429476224419'),
 (original,'boot.bin',4,'d0565e4ef0f59aa08ad0be91ffae61ceb85aedff5f36f346c95312b0f9407ac6'),
 (original,'code.bin',14428,'e33cb65be5c53d77f486ee7ea2371a2ffc78638e5ed72bfa8b2fe2bf8cf19736')):
 data=(directory/name).read_bytes();assert len(data)==size and hashlib.sha256(data).hexdigest()==digest
with tempfile.TemporaryDirectory(prefix='pioneer-dense-partition-test-',dir='/private/tmp') as temporary:
 binary=pathlib.Path(temporary)/'test'
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-ffp-contract=off','-Wall','-Wextra','-Werror',
                 '-fsanitize=address,undefined',str(here/'pioneer_q38_dense_partition_image_test.cpp'),'-o',str(binary)],check=True)
 subprocess.run([str(binary),str(image),str(original),*sys.argv[3:]],check=True,
                env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
