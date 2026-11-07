#!/usr/bin/env python3
"""Check the actual hybrid upload-to-token client without device access."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile

assert __debug__ and len(sys.argv)==2
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
sdk=Path(sys.argv[1]);assert sdk.is_absolute() and sdk.is_dir()
here=Path(__file__).resolve().parent;root=here.parent;compiler=os.environ.get('CXX','c++')
def run(*args):subprocess.run([str(v) for v in args],check=True)
with tempfile.TemporaryDirectory(prefix='pioneer-hybrid-full-session-',dir='/private/tmp') as directory:
 out=Path(directory);binary=out/'test'
 run(compiler,'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-Wno-unused-function','-Wno-unused-variable',
  '-fsanitize=address,undefined',here/'pioneer_q38_hybrid_full_session_test.cpp','-o',binary)
 for args in ((),('--long',),('--cancel',),('--bad-record',),('--bad-support',),('--source-lost',)):
  run(binary,*args)
 for mode in ('--fail','--bad'):
  for action in range(1,7):run(binary,mode,action)
 fixed=out/'fixed-test'
 run(compiler,'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-Wno-unused-function','-Wno-unused-variable',
  '-fsanitize=address,undefined','-DPIONEER_Q38_HYBRID_FIXED_126=1',here/'pioneer_q38_hybrid_full_session_test.cpp','-o',fixed)
 run(fixed);run(fixed,'--fail',1);run(fixed,'--bad',1)
 # Separate full candidate cannot be silently turned into validation, Work,
 # a kernel, an old full client, or the untracked upload diagnostic.
 header=root/'PioneerQ38HybridFullSession.h'
 source='#include "'+str(header)+'"\n'
 valid=['-DPIONEER_Q38_HYBRID_FULL_CLIENT=1','-DPIONEER_Q38_HYBRID_CANDIDATE_UPLOAD_CLIENT=1',
  '-DPIONEER_Q38_HYBRID_TRACKED_UPLOAD_CLIENT=1','-DPIONEER_Q38_HYBRID_UPLOAD_DIAGNOSTIC=1',
  '-DPIONEER_Q38_GDN_MUTABLE_STATE=1','-DPIONEER_BANKS_MOCK_ONLY=1','-DB_DEVICE_OP_CODES_END=1000']
 for index in range(3):
  result=subprocess.run([compiler,'-std=c++17',*(v for i,v in enumerate(valid) if i!=index),'-E','-x','c++','-'],
   input=source,capture_output=True,text=True)
  assert result.returncode and 'Hybrid full candidate is a separate' in result.stderr
 for forbidden in ('_KERNEL_MODE','__FAST_MATH__','PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_FULL_MODEL_CPU_REFERENCE',
  'PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE','PIONEER_Q38_FULL_OFFLOAD_CLIENT',
  'PIONEER_Q38_FULL_OFFLOAD_NATIVE','PIONEER_Q38_BF16_FULL_CLIENT'):
  result=subprocess.run([compiler,'-std=c++17',*valid,'-D'+forbidden+'=1','-E','-x','c++','-'],
   input=source,capture_output=True,text=True)
  assert result.returncode and 'Hybrid full candidate is a separate' in result.stderr,forbidden
 run(sys.executable,root/'build_q38_hybrid_full_diagnostic.py',sdk,out/'cross-build')
 run(sys.executable,root/'build_q38_hybrid_full_diagnostic.py','--fixed-126',sdk,out/'fixed-cross-build')
 print('PASS hybrid full client lifecycle and Haiku cross-build; simulated only, no physical inference or speed')
