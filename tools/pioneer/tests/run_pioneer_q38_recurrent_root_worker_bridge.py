#!/usr/bin/env python3
"""Connect actual RV32 ROOT job bytes to a separate worker simulator; no card."""
import hashlib
import os
from pathlib import Path
import stat
import subprocess
import sys
import tempfile

assert (len(sys.argv) in (6,7) and (len(sys.argv)==6 or sys.argv[6] in ('--gate','--qkv'))) or (len(sys.argv)==9 and sys.argv[6] in ('--full','--full-gate','--full-qkv'))
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
sdk,root,worker,leaf,private=map(Path,sys.argv[1:6]);here=Path(__file__).resolve().parent
mode=sys.argv[6] if len(sys.argv)==7 else '--gate' if len(sys.argv)==9 and sys.argv[6]=='--full-gate' else '--qkv' if len(sys.argv)==9 and sys.argv[6]=='--full-qkv' else '--out'
geometry={'--gate':(6144,5120),'--qkv':(10240,5120),'--out':(5120,6144)}
full=Path(sys.argv[7]) if len(sys.argv)==9 else None
if full:
    assert full.is_absolute() and full.is_file() and not full.is_symlink() and len(sys.argv[8])==64
    info=full.lstat();assert info.st_uid==os.geteuid() and info.st_nlink==1 and stat.S_IMODE(info.st_mode)==0o600
    rows,width=geometry[mode];assert info.st_size==32+(rows//32)*((width//32)*1088+128)
    assert hashlib.sha256(full.read_bytes()).hexdigest()==sys.argv[8]
roles={'--gate':5,'--qkv':6,'--out':13}
sample_pins={'--out':('1facbbadaef22f9b1fadaa33a4b19f2168bc9b1a244e9df9d7880ddd97b10cb1','7cef223c05f1e78b0a4c7c45cd24327b569a7d7e9cd3196895151ab67a18d65d'),
             '--gate':('c4781e0c6c3963ff1e1b3ac36c1d21fbb1dd74e2785826489cf67cbb139c9f36','a5fadca5d6baeaa921472989cf038dd4aa2a90a666f4c7b3262c8ee141841ff2'),
             '--qkv':('396fac17c3a837d7b440fb2982e4748e8c06c6c7fbc8299acdade184ec268a8a','a81f5268d942344b4c4cfd4d0d02853dc82f36bfe8ab099a5bfa5d47d94de86d')}
assert all(p.is_absolute() and p.is_dir() and not p.is_symlink() for p in (sdk,root,worker,leaf,private))
assert private.stat().st_uid==os.geteuid() and stat.S_IMODE(private.stat().st_mode)==0o700
pins=((root/'program.elf','c69b1aa610725fabfe37e28499082eb6b0abae04c556510bad376dfccf414d24'),
      (root/'bf16.bin','296ab69ff0cf2c2526ecfd606c3aa7e0eeaa6eea795bb3623eb8e5eaf08a4f9f'),
      (root/'text.bin','4ed3fe2e1548c93f957ccb57c887ae4e75bac452108071a6c32de61a2b9ad1b9'),
      (root/'terminal.bin','6eb1d84addfac7202ad4f0d747c6e64d3b92c03025651c7dabe6c56c9cba95d5'),
      (root/'conv.bin','8feb9b7f7f5cdb81a4eb0802e0bb6ad99b1fb43feaee6d9db1895394f1cd2fd3'),
      (worker/'program.elf','e83749ca5a7dc9d66983b9a80e0d6868079b12d8defdf31e40708235fb80366d'),
      (leaf/'program.elf','e8a04408d0c1a2d0ffa90d1d92ca77f3bb56f849c1d714723dfae9f3110ba72f'),
      (private/'PioneerNativeBfpModelBandData.h',sample_pins[mode][0]),
      (private/'PioneerNativeBfpModelBandMetadata.h',sample_pins[mode][1]))
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
    for binary,source,selected_roles in ((rootbin,'pioneer_q38_recurrent_root_job_bridge.cpp',root_roles),
                                (workerbin,'pioneer_q38_recurrent_worker_job_bridge.cpp',worker_roles)):
        subprocess.run([compiler,*common,*['-D'+r+'=1' for r in selected_roles],'-I'+str(private),str(here/source),'-o',str(binary)],check=True)
    boot=subprocess.check_output([str(workerbin),str(worker),str(leaf),'--boot'],env=env);assert len(boot)==119*256
    job=subprocess.run([str(rootbin),str(root),str(roles[mode])],input=boot,stdout=subprocess.PIPE,check=True,env=env).stdout
    chunks=geometry[mode][1]//256;assert len(job)==chunks*512+119*256
    runargs=[str(workerbin),str(worker),str(leaf),'--run-full',str(full)] if full else [str(workerbin),str(worker),str(leaf),'--run']
    subprocess.run(runargs,input=job,check=True,env=env)
    badboot=bytearray(boot);badboot[0]^=1
    rejected=subprocess.run([str(rootbin),str(root),str(roles[mode])],input=badboot,stdout=subprocess.PIPE,stderr=subprocess.PIPE,env=env)
    assert rejected.returncode and not rejected.stdout
    badproducer=bytearray(job);badproducer[256+28*4]=7
    rejected=subprocess.run(runargs,input=badproducer,stdout=subprocess.PIPE,stderr=subprocess.PIPE,env=env)
    assert rejected.returncode and not rejected.stdout
    badjob=bytearray(job);badjob[chunks*512+34*4]=7
    rejected=subprocess.run(runargs,input=badjob,stdout=subprocess.PIPE,stderr=subprocess.PIPE,env=env)
    assert rejected.returncode and not rejected.stdout
for path,digest in pins:assert hashlib.sha256(path.read_bytes()).hexdigest()==digest
if full:assert hashlib.sha256(full.read_bytes()).hexdigest()==sys.argv[8]
print('PASS invalid simulated boot and wrong-role producer/matrix jobs rejected; NO DEVICE')
