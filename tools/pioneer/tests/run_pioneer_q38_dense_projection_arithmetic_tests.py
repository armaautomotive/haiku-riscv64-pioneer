#!/usr/bin/env python3
"""Pinned actual QDM1+matrix peers; synthetic endpoints, NOT native or speed."""
import hashlib
import os
import pathlib
import re
import subprocess
import sys
import tempfile
from pioneer_q38_resident_continue_image_common import verify
assert len(sys.argv)==3
here=pathlib.Path(__file__).resolve().parent;image=pathlib.Path(sys.argv[1]);peer=verify(sys.argv[2])
for name,size,digest in (
    ('boot',4,'e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b'),
    ('text',7520,'2f69045e8bb3b7c8348cb604254f73ff6673af821a407d4d889d5b589335334e'),
    ('prefix',6424,'a22a00de78c13e1f2cb4bcefabe0ed0b88fee9261fc76f6ee65c797da55c2408')):
    data=(image/(name+'.bin')).read_bytes();assert len(data)==size and hashlib.sha256(data).hexdigest()==digest
flags=re.findall(r'-D[A-Z][A-Z0-9_]*(?:=[0-9]+)?',(here/'run_pioneer_q38_ffn_persistent_image_tests.sh').read_text())
flags+=['-DPIONEER_Q38_DENSE_PROJECTION_IMAGE=1','-DPIONEER_Q38_COMPACT_ACTIVATIONS','-DPIONEER_Q38_REUSE_ACTIVATIONS',
        '-DPIONEER_Q38_WORK_SPARSE_EXPORT','-DPIONEER_Q38_WORK_COMPACT_OUTPUT']
with tempfile.TemporaryDirectory(prefix='pioneer-dense-projection-arithmetic-',dir='/private/tmp') as temp:
    binary=pathlib.Path(temp)/'test'
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-ffp-contract=off','-Wall','-Wextra','-Werror',
                    '-fsanitize=address,undefined',*flags,str(here/'pioneer_q38_dense_projection_arithmetic_test.cpp'),'-o',str(binary)],check=True)
    subprocess.run([str(binary),str(image),str(peer)],check=True,
                   env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
