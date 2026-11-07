#!/usr/bin/env python3
"""Build and test the QGW7 model coordinator off-device; never install it."""
import hashlib
import os
from pathlib import Path
import subprocess
import sys
import tempfile

assert __debug__ and len(sys.argv) in (2,4) and not os.environ.get('PYTHONOPTIMIZE')
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
sdk=Path(sys.argv[1]);assert sdk.is_absolute() and sdk.is_dir()
worker=leaf=None
if len(sys.argv)==4:
 worker,leaf=map(Path,sys.argv[2:]);assert all(p.is_absolute() and p.is_dir() for p in (worker,leaf))
here=Path(__file__).resolve().parent;root=here.parent
sys.path.insert(0,str(root));from PioneerQ38HybridProgramPins import ROOT_PINS
flags=['--inspect-candidate','--cancel-boundary','--f32-hidden','--unroll-dense-seals','--boundary-seals',
 '--unroll-quiet','--worker-local-source-guards','--precise-activation','--single-closed-audit',
 '--worker-sealed-admission','--bf16-fpu','--hybrid-bfp']
def run(*args,quiet=False):
 subprocess.run([str(a) for a in args],check=True,stdout=subprocess.DEVNULL if quiet else None)
def pins(folder):
 return tuple((p.name,p.stat().st_size,hashlib.sha256(p.read_bytes()).hexdigest())
  for p in (folder/name for name in ('boot.bin','text.bin','terminal.bin','conv.bin','bf16.bin','program.elf')))
with tempfile.TemporaryDirectory(prefix='pioneer-qgw7-root-',dir='/private/tmp') as tmp:
 tmp=Path(tmp);builder=root/'build_qwen38_token_root.py'
 for extra in (['--hybrid-matrix-attention'],[*flags,'--hybrid-matrix-attention','--fast-rms'],
               [*flags,'--hybrid-matrix-attention','--hybrid-reuse-input'],
               [*flags,'--hybrid-matrix-attention','--bf16-profile']):
  refused=tmp/'refused';result=subprocess.run([sys.executable,str(builder),str(sdk),str(refused),*extra],capture_output=True)
  assert result.returncode and not refused.exists()
 old=tmp/'old';first=tmp/'first';second=tmp/'second'
 run(sys.executable,builder,sdk,old,*flags,quiet=True)
 assert pins(old)==ROOT_PINS,'existing QGW5 ROOT changed'
 for image in (first,second):run(sys.executable,builder,sdk,image,*flags,'--hybrid-matrix-attention',quiet=True)
 assert pins(first)==pins(second),'QGW7 ROOT did not independently reproduce'
 assert pins(first)!=pins(old),'QGW7 cannot share the old ROOT identity'
 nm=str(sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku-nm')
 for image in (first,old):
  (image/'program.demangled').write_bytes(subprocess.check_output([nm,'-nC',str(image/'program.elf')]))
 binary=tmp/'actual-root'
 run(os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-ffp-contract=off',
  '-fsanitize=address,undefined','-DPIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION=1',
  '-DPIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT=1',here/'pioneer_q38_hybrid_actual_root_test.cpp','-o',binary)
 run(binary,first)
 if worker:
  expected={
   'boot.bin':'a12af9d63593a1f882750a147dd7a511ac476c5c7a55526bd4d51c685173991c',
   'globalworker.bin':'9d18659a0c413f5ee14c51887f2d572c111a8d24b71e827b798de3ea5ff146bf',
   'bf16producer.bin':'8b3e477d51eeca467e155b72234bed728c7109947f78cfb4361ce6dbc7afb2c3',
   'dense.bin':'e0adac5fafa542351877d41f05180afa13ae63a3d0ade7db5849755f1b67b0e6',
   'cachedfpu.bin':'bcc704d361936935e26c57b138693accc5a22207ab5e45019d78baec1385abc9',
   'ffn.bin':'c1dc115dadb20b1ee1d18fb04ce61f15a1d1644a523ccfec8fea70dbd5164e45',
   'service.bin':'3b3cdbd88d049b3f0c72016d8eccfad7eab3fd6861a5353a50f26d7d4b46795f',
   'affine.bin':'47c52fee6ccfb0a8847fe17a9d43d4016e7cfc6c68cc1427673f6869cd287b78',
   'text.bin':'2548ed0d4fdb68ce48325cd6f8581bff6bdc5b34a87f3ba217e36e4a37ed6e5c',
   'gdn.bin':'dd982ac614d163795442942529ef9f4e5519552097b438f5fc4464b40eab3436',
   'program.elf':'157ed4d4563b2e131fbd48c48e776d11d36e5d7c3097570b2434054ce85016b5'}
  for name,digest in expected.items():assert hashlib.sha256((worker/name).read_bytes()).hexdigest()==digest,name
  assert (worker/'program.symbols').read_bytes()==subprocess.check_output([nm,'-n',str(worker/'program.elf')])
  for name,digest in (('boot.bin','d47a1c29c265663f5bd4bd49aaf28248b7345d7feb5a4f770caa27315deaa4dd'),
                      ('text.bin','04d43a4e5ad5b766fea07d0d3f9f48168557abda4772826d804f643db644faaf')):
   assert hashlib.sha256((leaf/name).read_bytes()).hexdigest()==digest,name
  connected=tmp/'root-worker'
  run(os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-ffp-contract=off',
   '-fsanitize=address,undefined','-DPIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION=1',
   '-DPIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT=1',here/'pioneer_q38_hybrid_matrix_root_worker_test.cpp','-o',connected)
  run(connected,first,old,worker,leaf)
 else:
  print('SKIP coordinator-to-worker handoff: supply QGW7 worker and BFP4 leaf directories')
 print('PASS QGW7 ROOT independent builds match; QGW5 pins unchanged; four incompatible profiles refused')
 for pin in pins(first):print(pin)
 print('OFFLINE ONLY: no full lifecycle, real weights, device execution, native/Work authorization or measured TPS')
