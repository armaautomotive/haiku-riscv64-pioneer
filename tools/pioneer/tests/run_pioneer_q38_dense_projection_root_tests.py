#!/usr/bin/env python3
"""NEW pinned QDM1 root tests;synthetic peers are NOT native/math authority."""
import hashlib
import os
import pathlib
import subprocess
import sys
import tempfile
assert len(sys.argv)==2
here=pathlib.Path(__file__).resolve().parent;image=pathlib.Path(sys.argv[1])
for name,size,digest in (
    ('boot',4,'e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b'),
    ('text',7520,'2f69045e8bb3b7c8348cb604254f73ff6673af821a407d4d889d5b589335334e'),
    ('prefix',6424,'a22a00de78c13e1f2cb4bcefabe0ed0b88fee9261fc76f6ee65c797da55c2408')):
    data=(image/(name+'.bin')).read_bytes();assert len(data)==size and hashlib.sha256(data).hexdigest()==digest
flags=['PIONEER_Q38_DENSE_PROJECTION_IMAGE=1','PIONEER_Q38_COMPACT_ACTIVATIONS','PIONEER_Q38_REUSE_ACTIVATIONS',
       'PIONEER_Q38_WORK_COHORT_CACHE','PIONEER_Q38_WORK_SKIP_DOT_CHECK','PIONEER_Q38_WORK_SPARSE_EXPORT','PIONEER_Q38_WORK_COMPACT_OUTPUT']
with tempfile.TemporaryDirectory(prefix='pioneer-dense-projection-root-',dir='/private/tmp') as temp:
    for source,args in (('pioneer_q38_dense_projection_plan_test.cpp',[]),('pioneer_q38_dense_projection_root_test.cpp',[str(image)])):
        binary=pathlib.Path(temp)/source
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-ffp-contract=off','-Wall','-Wextra','-Werror',
                        '-fsanitize=address,undefined',*['-D'+f for f in flags],str(here/source),'-o',str(binary)],check=True)
        subprocess.run([str(binary),*args],check=True,env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
