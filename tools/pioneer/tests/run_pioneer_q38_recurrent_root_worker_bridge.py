#!/usr/bin/env python3
"""Connect actual RV32 ROOT job bytes to a separate worker simulator; no card."""
import hashlib
import os
from pathlib import Path
import stat
import subprocess
import sys
import tempfile

assert len(sys.argv)==6 and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
sdk,root,worker,leaf,private=map(Path,sys.argv[1:]);here=Path(__file__).resolve().parent
assert all(p.is_absolute() and p.is_dir() and not p.is_symlink() for p in (sdk,root,worker,leaf,private))
assert private.stat().st_uid==os.geteuid() and stat.S_IMODE(private.stat().st_mode)==0o700
pins=((root/'program.elf','c69b1aa610725fabfe37e28499082eb6b0abae04c556510bad376dfccf414d24'),
      (root/'bf16.bin','296ab69ff0cf2c2526ecfd606c3aa7e0eeaa6eea795bb3623eb8e5eaf08a4f9f'),
      (root/'text.bin','4ed3fe2e1548c93f957ccb57c887ae4e75bac452108071a6c32de61a2b9ad1b9'),
      (root/'terminal.bin','6eb1d84addfac7202ad4f0d747c6e64d3b92c03025651c7dabe6c56c9cba95d5'),
      (root/'conv.bin','8feb9b7f7f5cdb81a4eb0802e0bb6ad99b1fb43feaee6d9db1895394f1cd2fd3'),
      (worker/'program.elf','e83749ca5a7dc9d66983b9a80e0d6868079b12d8defdf31e40708235fb80366d'),
      (leaf/'program.elf','e8a04408d0c1a2d0ffa90d1d92ca77f3bb56f849c1d714723dfae9f3110ba72f'),
      (private/'PioneerNativeBfpModelBandData.h','1facbbadaef22f9b1fadaa33a4b19f2168bc9b1a244e9df9d7880ddd97b10cb1'),
      (private/'PioneerNativeBfpModelBandMetadata.h','7cef223c05f1e78b0a4c7c45cd24327b569a7d7e9cd3196895151ab67a18d65d'))
for path,digest in pins:
    assert path.is_file() and not path.is_symlink() and hashlib.sha256(path.read_bytes()).hexdigest()==digest,path
for path,_ in pins[-2:]:
    info=path.lstat();assert info.st_uid==os.geteuid() and info.st_nlink==1 and stat.S_IMODE(info.st_mode)==0o600
compiler=os.environ.get('CXX','c++')
common=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-Wno-unused-function','-Wno-unused-variable',
        '-Wno-misleading-indentation','-ffp-contract=off','-fsanitize=address,undefined']
root_roles=('PIONEER_Q38_RECURRENT_BFP8_WIDTH_INSPECTION','PIONEER_Q38_QGWD_PLAN_SIMULATION',
 'PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION','PIONEER_Q38_HEAD_MATRIX_SIMULATION','PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION',
 'PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT','PIONEER_Q38_DENSE_Q8_MATRIX_WORKER_INSPECTION',
 'PIONEER_Q38_QGWE_BATCH_DENSE_PLAN','PIONEER_Q38_QGWF_DUAL_BATCH_PLAN')
worker_roles=('PIONEER_Q38_RECURRENT_BFP8_WIDTH_INSPECTION','PIONEER_Q38_QGWF_BATCH4_WORKER',
 'PIONEER_Q38_QGWE_BATCH_DENSE_WORKER','PIONEER_Q38_QGW8_BFP8_BATCH_WORKER','PIONEER_Q38_BFP4_BATCH_INPUT_EXPERIMENT',
 'PIONEER_Q38_BFP8_BATCH_INPUT_EXPERIMENT','PIONEER_Q38_QGWD_SRCA_DENSE_Q8_WORKER','PIONEER_Q38_DENSE_Q8_MATRIX_WORKER_INSPECTION',
 'PIONEER_Q38_DENSE_BF16_SRCA_BASE_CANDIDATE','PIONEER_Q38_HEAD_MATRIX_WORKER_INSPECTION','PIONEER_Q38_HYBRID_WORKER_IMAGE',
 'PIONEER_Q38_BF16_WORKER_IMAGE','PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION','PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT')
env={**os.environ,'UBSAN_OPTIONS':'halt_on_error=1'}
with tempfile.TemporaryDirectory(prefix='q38-root-worker-bridge-',dir='/private/tmp') as tmp:
    rootbin,workerbin=Path(tmp)/'root',Path(tmp)/'worker'
    tool=str(sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku-')
    assert subprocess.check_output([tool+'nm','-nC',str(root/'program.elf')])==(root/'program.demangled').read_bytes()
    assert subprocess.check_output([tool+'nm','-n',str(worker/'program.elf')])==(worker/'program.symbols').read_bytes()
    for image,sections in ((root,('bf16','text','terminal','conv')),
                           (worker,('boot','globalworker','bf16producer','dense','cachedfpu','ffn','service','affine','text','gdn')),
                           (leaf,('boot','text'))):
        for section in sections:
            output=Path(tmp)/'section.bin'
            subprocess.run([tool+'objcopy','-O','binary','-j','.'+section,str(image/'program.elf'),str(output)],check=True)
            assert output.read_bytes()==(image/(section+'.bin')).read_bytes(),(image,section)
    for binary,source,roles in ((rootbin,'pioneer_q38_recurrent_root_job_bridge.cpp',root_roles),
                                (workerbin,'pioneer_q38_recurrent_worker_job_bridge.cpp',worker_roles)):
        subprocess.run([compiler,*common,*['-D'+r+'=1' for r in roles],'-I'+str(private),str(here/source),'-o',str(binary)],check=True)
    boot=subprocess.check_output([str(workerbin),str(worker),str(leaf),'--boot'],env=env);assert len(boot)==119*256
    job=subprocess.run([str(rootbin),str(root)],input=boot,stdout=subprocess.PIPE,check=True,env=env).stdout;assert len(job)==119*256
    subprocess.run([str(workerbin),str(worker),str(leaf),'--run'],input=job,check=True,env=env)
    badboot=bytearray(boot);badboot[0]^=1
    rejected=subprocess.run([str(rootbin),str(root)],input=badboot,stdout=subprocess.PIPE,stderr=subprocess.PIPE,env=env)
    assert rejected.returncode and not rejected.stdout
    badjob=bytearray(job);badjob[34*4]=5
    rejected=subprocess.run([str(workerbin),str(worker),str(leaf),'--run'],input=badjob,stdout=subprocess.PIPE,stderr=subprocess.PIPE,env=env)
    assert rejected.returncode and not rejected.stdout
for path,digest in pins:assert hashlib.sha256(path.read_bytes()).hexdigest()==digest
print('PASS invalid simulated boot and wrong-role job rejected; NO DEVICE')
