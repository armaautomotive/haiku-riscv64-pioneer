#!/usr/bin/env python3
"""Connect real emitted QGW7 worker and ROOT instructions in a synthetic NoC."""
import hashlib
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

assert __debug__ and len(sys.argv)==6 and not os.environ.get('PYTHONOPTIMIZE')
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
sdk,old,new,worker,leaf=map(Path,sys.argv[1:])
assert all(p.is_absolute() and p.is_dir() for p in (sdk,old,new,worker,leaf))
here=Path(__file__).resolve().parent
expected={
 old:{'program.elf':'f9be15a56b126b8809880eb3dee32217c4c6cfa41f7e546714add7747b814c83',
      'boot.bin':'32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca',
      'text.bin':'ecbc0ccce592a08430a67a9fbd8d567c6528f7fcce56e9d44d6ed7fb2030f314',
      'terminal.bin':'6eb1d84addfac7202ad4f0d747c6e64d3b92c03025651c7dabe6c56c9cba95d5',
      'conv.bin':'6f917fdaf3df5870c337f85a8d17d14f79a9c6fda8362b4425eabfab62f58801',
      'bf16.bin':'b9cc6b376083e5c082ec3cf3e69d13dda742d5ed6693fe24164d3601907f96c8'},
 new:{'program.elf':'567e04f17e465deb634d51d2469f732d40995acc8343399148b2f37629a56f5b',
      'boot.bin':'32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca',
      'text.bin':'6014d392b8103a5ac0d9bf5c15704dc75ebe0c0a43a22d58f041c05cc534ca9b',
      'terminal.bin':'6eb1d84addfac7202ad4f0d747c6e64d3b92c03025651c7dabe6c56c9cba95d5',
      'conv.bin':'2b3baac417c2c672b516e863409b166b34413d0e5952798e24c17912c2916705',
      'bf16.bin':'6ee1c1ac9cae9059933eb1938fabd6e9b471bdc67aab2497048d1f023c6ae756'},
 worker:{'boot.bin':'a12af9d63593a1f882750a147dd7a511ac476c5c7a55526bd4d51c685173991c',
         'globalworker.bin':'02969656a32de00780f767dff21dce1226cc9dd404bf46ad3e2156a52c2a1db2',
         'bf16producer.bin':'8b3e477d51eeca467e155b72234bed728c7109947f78cfb4361ce6dbc7afb2c3',
         'dense.bin':'d40bb04efbd330252fb679691f7cc65d3a9fd2e15ed051ce21e557e9d3e93c0d',
         'cachedfpu.bin':'bcc704d361936935e26c57b138693accc5a22207ab5e45019d78baec1385abc9',
         'ffn.bin':'c1dc115dadb20b1ee1d18fb04ce61f15a1d1644a523ccfec8fea70dbd5164e45',
         'service.bin':'3b3cdbd88d049b3f0c72016d8eccfad7eab3fd6861a5353a50f26d7d4b46795f',
         'affine.bin':'47c52fee6ccfb0a8847fe17a9d43d4016e7cfc6c68cc1427673f6869cd287b78',
         'text.bin':'2548ed0d4fdb68ce48325cd6f8581bff6bdc5b34a87f3ba217e36e4a37ed6e5c',
         'gdn.bin':'dd982ac614d163795442942529ef9f4e5519552097b438f5fc4464b40eab3436',
         'program.elf':'3b1a85fb54b82445732fef91e795bc6c1ba0a950eac6ab6d7ac2da5128767565'},
 leaf:{'boot.bin':'d47a1c29c265663f5bd4bd49aaf28248b7345d7feb5a4f770caa27315deaa4dd',
       'text.bin':'04d43a4e5ad5b766fea07d0d3f9f48168557abda4772826d804f643db644faaf'}}
for folder,items in expected.items():
 for name,digest in items.items():assert hashlib.sha256((folder/name).read_bytes()).hexdigest()==digest,(folder,name)
nm=str(sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku-nm')
assert (worker/'program.symbols').read_bytes()==subprocess.check_output([nm,'-n',str(worker/'program.elf')])
def symbols(folder):
 rows=[]
 for line in subprocess.check_output([nm,'-nSC',str(folder/'program.elf')],text=True).splitlines():
  p=line.split(maxsplit=3)
  if len(p)==4:
   try:rows.append((int(p[0],16),int(p[1],16),p[3]))
   except ValueError:pass
 return rows
oldrows,newrows=symbols(old),symbols(new)
def matching(rows,name):return [(a,s) for a,s,label in rows if label.startswith(name+'(')]
oldmatrix=matching(oldrows,'Projection::BfGatherMatrix');newmatrix=matching(newrows,'Projection::BfGatherMatrix')
queue=matching(newrows,'PioneerQ38CounterGather::Read')
assert oldmatrix==[(0x234a0,0x334),(0x26a50,0x334)]
assert newmatrix==[(0x23cb0,0x290),(0x26940,0x334)] and queue==[(0x282d8,0x350)]
with tempfile.TemporaryDirectory(prefix='pioneer-qgw7-matrix-gather-',dir='/private/tmp') as directory:
 tmp=Path(directory);roots=[]
 # The queued image must never be silently accepted as the existing tag127 ROOT.
 rejected=tmp/'rejected-header'
 result=subprocess.run([sys.executable,str(here.parent/'generate_qwen38_hybrid_programs.py'),
  '--matrix-attention-inspection',str(new),str(worker),str(rejected)],capture_output=True)
 assert result.returncode and not rejected.exists()
 for image in (old,new):
  copy=tmp/('old' if image==old else 'new');copy.mkdir();roots.append(copy)
  for name in expected[image]:shutil.copyfile(image/name,copy/name)
  (copy/'program.demangled').write_bytes(subprocess.check_output([nm,'-nC',str(copy/'program.elf')]))
 entries=tmp/'entries';entries.write_text(f'{oldmatrix[0][0]:x} {newmatrix[0][0]:x} {queue[0][0]:x} {queue[0][1]:x}\n')
 binary=tmp/'test'
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror',
  '-ffp-contract=off','-fsanitize=address,undefined','-DPIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION=1',
  '-DPIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT=1',str(here/'pioneer_q38_hybrid_queued_matrix_gather_test.cpp'),
  '-o',str(binary)],check=True)
 subprocess.run([str(binary),str(roots[0]),str(roots[1]),str(worker),str(leaf),str(entries)],check=True,
  env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
