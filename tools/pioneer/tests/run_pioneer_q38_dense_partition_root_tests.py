#!/usr/bin/env python3
"""Actual ROOT+96workers+4QDP; byte fabric ONLY, not physical model speed."""
import hashlib
import os
import pathlib
import subprocess
import sys
import tempfile
assert len(sys.argv) in (5,6) and (len(sys.argv)==5 or sys.argv[5]=='--full')
root,worker,qdp,qda=map(pathlib.Path,sys.argv[1:5]);here=pathlib.Path(__file__).resolve().parent
for directory,name,size,digest in (
 (root,'boot.bin',4,'49beea42a964dc73343fca928d59be0eb6a6d28e4c1354b47894f3f60ef4a31a'),
 (root,'code.bin',12140,'a8aa43b88af5d821a50498d0e79a75025ca672490fa6d4ead51be8442013bf8c'),
 (worker,'boot.bin',4,'32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca'),
 (worker,'code.bin',10804,'ecc4bd29fa2517a61737ad67e59cfce942b6363f96370774648a429476224419'),
 (qdp,'code.bin',4836,'ff39e984f76fe0d0591e9a48ea1137a231ecdaf2390b14223a8a8922b8e2488a'),
 (qda,'code.bin',14428,'e33cb65be5c53d77f486ee7ea2371a2ffc78638e5ed72bfa8b2fe2bf8cf19736')):
 data=(directory/name).read_bytes();assert len(data)==size and hashlib.sha256(data).hexdigest()==digest
with tempfile.TemporaryDirectory(prefix='pioneer-dense-partition-root-test-',dir='/private/tmp') as temporary:
 binary=pathlib.Path(temporary)/'test'
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-ffp-contract=off','-Wall','-Wextra','-Werror',
                 '-fsanitize=address,undefined',str(here/'pioneer_q38_dense_partition_root_test.cpp'),'-o',str(binary)],check=True)
 subprocess.run([str(binary),str(root),str(worker),str(qdp),str(qda),*sys.argv[5:]],check=True,
                env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
