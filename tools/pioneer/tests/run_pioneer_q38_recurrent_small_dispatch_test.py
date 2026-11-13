#!/usr/bin/env python3
"""Pinned offline alpha/beta shared-dispatch check; never opens hardware."""
import hashlib
import os
from pathlib import Path
import stat
import subprocess
import sys
import tempfile

assert __debug__ and len(sys.argv)==7
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
sdk,worker,leaf,private,alpha,beta=map(Path,sys.argv[1:]);here=Path(__file__).resolve().parent
assert sdk.is_absolute() and sdk.is_dir() and not sdk.is_symlink()
pins=((worker/'program.elf','67a9bf22e0f9713b699bb058db3cc5702f4940fb786845e66a3e89a3bfe2175d'),
      (leaf/'program.elf','e8a04408d0c1a2d0ffa90d1d92ca77f3bb56f849c1d714723dfae9f3110ba72f'),
      (private/'PioneerNativeBfpModelBandData.h','396fac17c3a837d7b440fb2982e4748e8c06c6c7fbc8299acdade184ec268a8a'),
      (private/'PioneerNativeBfpModelBandMetadata.h','a81f5268d942344b4c4cfd4d0d02853dc82f36bfe8ab099a5bfa5d47d94de86d'),
      (alpha,'92f7287e9626c4dae6ff4c1118d6d2c64f194cf1c239ac3f7b1b60e477ed4fbb'),
      (beta,'f6615b16825dbe365aad7d01729b2b6cbd19c2f0d51545890d1fe74933bcb753'))
for path,digest in pins:
    assert path.is_absolute() and path.is_file() and not path.is_symlink()
    assert hashlib.sha256(path.read_bytes()).hexdigest()==digest,path
for path in (pins[2][0],pins[3][0],alpha,beta):
    info=path.lstat();assert info.st_uid==os.geteuid() and info.st_nlink==1 and stat.S_IMODE(info.st_mode)==0o600
roles=('PIONEER_Q38_RECURRENT_BFP8_WIDTH_INSPECTION','PIONEER_Q38_RECURRENT_SMALL_BFP8_WORKER_INSPECTION',
 'PIONEER_Q38_QGWF_BATCH4_WORKER','PIONEER_Q38_QGWE_BATCH_DENSE_WORKER','PIONEER_Q38_QGW8_BFP8_BATCH_WORKER',
 'PIONEER_Q38_BFP4_BATCH_INPUT_EXPERIMENT','PIONEER_Q38_BFP8_BATCH_INPUT_EXPERIMENT','PIONEER_Q38_QGWD_SRCA_DENSE_Q8_WORKER',
 'PIONEER_Q38_DENSE_Q8_MATRIX_WORKER_INSPECTION','PIONEER_Q38_DENSE_BF16_SRCA_BASE_CANDIDATE',
 'PIONEER_Q38_HEAD_MATRIX_WORKER_INSPECTION','PIONEER_Q38_HYBRID_WORKER_IMAGE','PIONEER_Q38_BF16_WORKER_IMAGE',
 'PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION','PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT')
with tempfile.TemporaryDirectory(prefix='q38-small-dispatch-',dir='/private/tmp') as tmp:
    binary=Path(tmp)/'test'
    tool=str(sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku-')
    assert subprocess.check_output([tool+'nm','-n',str(worker/'program.elf')])==(worker/'program.symbols').read_bytes()
    for image,sections in ((worker,('boot','globalworker','bf16producer','cachedfpu','dense','text','affine','ffn','service','gdn')),
                           (leaf,('boot','text'))):
        for section in sections:
            extracted=Path(tmp)/'section.bin'
            subprocess.run([tool+'objcopy','-O','binary','-j','.'+section,str(image/'program.elf'),str(extracted)],check=True)
            assert extracted.read_bytes()==(image/(section+'.bin')).read_bytes(),(image,section)
    for name in ('pioneer_q38_recurrent_small_bfp8_dispatch_test.cpp','pioneer_q38_recurrent_small_schedule_test.cpp'):
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror',
                        '-Wno-unused-function','-Wno-unused-variable','-Wno-misleading-indentation','-ffp-contract=off',
                        '-fsanitize=address,undefined',*['-D'+r+'=1' for r in roles],'-I'+str(private),
                        str(here/name),'-o',str(binary)],check=True)
        subprocess.run([str(binary),str(worker),str(leaf),str(alpha),str(beta)],check=True,
                       env={**os.environ,'UBSAN_OPTIONS':'halt_on_error=1'})
for path,digest in pins:
    assert hashlib.sha256(path.read_bytes()).hexdigest()==digest,path
