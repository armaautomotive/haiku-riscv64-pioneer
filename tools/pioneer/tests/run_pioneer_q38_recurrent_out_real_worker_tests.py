#!/usr/bin/env python3
"""Simulate the compiled recurrent worker with private real-weight samples."""
import os
import hashlib
import stat
from pathlib import Path
import subprocess
import sys
import tempfile
if len(sys.argv) not in (5,6) or (len(sys.argv)==6 and sys.argv[5] not in ('--gate','--qkv')) or any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ):
    raise SystemExit('usage: ABS_SDK ABS_WORKER_IMAGE ABS_BFP8_LEAF ABS_PRIVATE_SAMPLES [--gate|--qkv]')
sdk,worker,leaf,private=map(Path,sys.argv[1:5]);here=Path(__file__).resolve().parent
mode=sys.argv[5] if len(sys.argv)==6 else '--out'
assert private.is_absolute() and private.is_dir() and not private.is_symlink() and private.stat().st_uid==os.geteuid() and stat.S_IMODE(private.stat().st_mode)==0o700
digests={'--out':('1facbbadaef22f9b1fadaa33a4b19f2168bc9b1a244e9df9d7880ddd97b10cb1','7cef223c05f1e78b0a4c7c45cd24327b569a7d7e9cd3196895151ab67a18d65d'),
         '--gate':('c4781e0c6c3963ff1e1b3ac36c1d21fbb1dd74e2785826489cf67cbb139c9f36','a5fadca5d6baeaa921472989cf038dd4aa2a90a666f4c7b3262c8ee141841ff2'),
         '--qkv':('396fac17c3a837d7b440fb2982e4748e8c06c6c7fbc8299acdade184ec268a8a','a81f5268d942344b4c4cfd4d0d02853dc82f36bfe8ab099a5bfa5d47d94de86d')}
pins=dict(zip(('PioneerNativeBfpModelBandData.h','PioneerNativeBfpModelBandMetadata.h'),digests[mode]))
for name,digest in pins.items():
    f=private/name;info=f.lstat();assert stat.S_ISREG(info.st_mode) and info.st_nlink==1 and info.st_uid==os.geteuid() and stat.S_IMODE(info.st_mode)==0o600 and hashlib.sha256(f.read_bytes()).hexdigest()==digest
assert all(p.is_absolute() and p.is_dir() and not p.is_symlink() for p in (sdk,worker,leaf))
assert hashlib.sha256((worker/'program.elf').read_bytes()).hexdigest()=='e83749ca5a7dc9d66983b9a80e0d6868079b12d8defdf31e40708235fb80366d'
assert hashlib.sha256((leaf/'program.elf').read_bytes()).hexdigest()=='e8a04408d0c1a2d0ffa90d1d92ca77f3bb56f849c1d714723dfae9f3110ba72f'
tool=sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku'
names=('PIONEER_Q38_RECURRENT_BFP8_WIDTH_INSPECTION','PIONEER_Q38_QGWF_BATCH4_WORKER',
       'PIONEER_Q38_QGWE_BATCH_DENSE_WORKER','PIONEER_Q38_QGW8_BFP8_BATCH_WORKER',
       'PIONEER_Q38_BFP4_BATCH_INPUT_EXPERIMENT','PIONEER_Q38_BFP8_BATCH_INPUT_EXPERIMENT',
       'PIONEER_Q38_QGWD_SRCA_DENSE_Q8_WORKER','PIONEER_Q38_DENSE_Q8_MATRIX_WORKER_INSPECTION',
       'PIONEER_Q38_DENSE_BF16_SRCA_BASE_CANDIDATE','PIONEER_Q38_HEAD_MATRIX_WORKER_INSPECTION',
       'PIONEER_Q38_HYBRID_WORKER_IMAGE','PIONEER_Q38_BF16_WORKER_IMAGE',
       'PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION','PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT')
with tempfile.TemporaryDirectory(prefix='q38-recurrent-connected-',dir='/private/tmp') as directory:
    tmp=Path(directory)
    assert (worker/'program.symbols').read_bytes()==subprocess.check_output([str(tool)+'-nm','-n',str(worker/'program.elf')])
    for section in ('boot','globalworker','bf16producer','dense','cachedfpu','ffn','service','affine','text','gdn'):
        out=tmp/(section+'.bin')
        subprocess.run([str(tool)+'-objcopy','-O','binary','-j','.'+section,str(worker/'program.elf'),str(out)],check=True)
        assert out.read_bytes()==(worker/(section+'.bin')).read_bytes(),section
    for section in ('boot','text'):
        out=tmp/('leaf-'+section+'.bin')
        subprocess.run([str(tool)+'-objcopy','-O','binary','-j','.'+section,str(leaf/'program.elf'),str(out)],check=True)
        assert out.read_bytes()==(leaf/(section+'.bin')).read_bytes(),section
    binary=tmp/'test'
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror',
                    '-Wno-unused-function','-Wno-unused-variable','-Wno-misleading-indentation','-ffp-contract=off',
                    '-fsanitize=address,undefined',*['-D'+name+'=1' for name in names],
                    '-I'+str(private),str(here/'pioneer_q38_recurrent_out_real_worker_test.cpp'),'-o',str(binary)],check=True)
    for sample in range(3):
        subprocess.run([str(binary),str(worker),str(leaf),str(sample)],check=True,env={**os.environ,'UBSAN_OPTIONS':'halt_on_error=1'})
    for name,digest in pins.items():assert hashlib.sha256((private/name).read_bytes()).hexdigest()==digest
