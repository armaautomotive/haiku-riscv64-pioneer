#!/usr/bin/env python3
"""Build and test the QGW7 model coordinator off-device; never install it."""
import hashlib
import os
from pathlib import Path
import subprocess
import sys
import tempfile

assert __debug__ and len(sys.argv)==2 and not os.environ.get('PYTHONOPTIMIZE')
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
sdk=Path(sys.argv[1]);assert sdk.is_absolute() and sdk.is_dir()
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
 (first/'program.demangled').write_bytes(subprocess.check_output([nm,'-nC',str(first/'program.elf')]))
 binary=tmp/'actual-root'
 run(os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-ffp-contract=off',
  '-fsanitize=address,undefined','-DPIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION=1',
  '-DPIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT=1',here/'pioneer_q38_hybrid_actual_root_test.cpp','-o',binary)
 run(binary,first)
 print('PASS QGW7 ROOT independent builds match; QGW5 pins unchanged; four incompatible profiles refused')
 for pin in pins(first):print(pin)
 print('OFFLINE ONLY: no full lifecycle, real weights, device execution, native/Work authorization or measured TPS')
