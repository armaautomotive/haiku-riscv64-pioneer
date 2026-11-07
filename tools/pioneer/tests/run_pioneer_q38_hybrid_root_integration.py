#!/usr/bin/env python3
"""Build the experimental actual ROOT and simulate its QGW5 job functions."""
import hashlib
import os
from pathlib import Path
import subprocess
import sys
import tempfile

assert __debug__ and len(sys.argv)==2 and not os.environ.get('PYTHONOPTIMIZE')
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
sdk=Path(sys.argv[1]);assert sdk.is_absolute() and sdk.is_dir()
here=Path(__file__).resolve().parent;root=here.parent;compiler=os.environ.get('CXX','c++')
sys.path.insert(0,str(root))
from PioneerQ38Bf16ProgramPins import ROOT_PINS
flags=['--inspect-candidate','--cancel-boundary','--f32-hidden','--unroll-dense-seals','--boundary-seals',
 '--unroll-quiet','--worker-local-source-guards','--precise-activation','--single-closed-audit',
 '--worker-sealed-admission','--bf16-fpu']
def run(*args,quiet=False):subprocess.run([str(a) for a in args],check=True,stdout=subprocess.DEVNULL if quiet else None)
with tempfile.TemporaryDirectory(prefix='pioneer-qgw5-actual-root-',dir='/private/tmp') as temporary:
 temp=Path(temporary);baseline=temp/'baseline';hybrid=temp/'hybrid';binary=temp/'test'
 run(sys.executable,root/'build_qwen38_token_root.py',sdk,baseline,*flags,quiet=True)
 for name,size,digest in ROOT_PINS:
  data=(baseline/name).read_bytes();assert len(data)==size and hashlib.sha256(data).hexdigest()==digest,name
 print('PASS unchanged BF16 ROOT pins; experimental flag does not alter baseline',flush=True)
 run(sys.executable,root/'build_qwen38_token_root.py',sdk,hybrid,*flags,'--hybrid-bfp',quiet=True)
 tool=str(sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku-')
 (hybrid/'program.demangled').write_bytes(subprocess.check_output([tool+'nm','-nC',str(hybrid/'program.elf')]))
 run(compiler,'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-ffp-contract=off',
  '-fsanitize=address,undefined',here/'pioneer_q38_hybrid_actual_root_test.cpp','-o',binary)
 run(binary,hybrid)
print('PASS experimental actual ROOT functions, unchanged baseline, offline simulator; NO device, full model or speed claim')
