#!/usr/bin/env python3
"""Exercise the tracked loader with the real kernel receipt off-device."""
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
with tempfile.TemporaryDirectory(prefix='pioneer-tracked-session-',dir='/private/tmp') as directory:
 out=Path(directory);binary=out/'test'
 run(compiler,'-std=c++17','-O2','-g','-Wall','-Wextra','-Werror','-Wno-unused-function','-Wno-unused-variable',
  '-fsanitize=address,undefined',here/'pioneer_q38_hybrid_tracked_session_test.cpp','-o',binary)
 run(binary);run(binary,'--seal-failure');run(binary,'--bad-seal')
 candidate=out/'candidate-test'
 run(compiler,'-std=c++17','-O2','-g','-Wall','-Wextra','-Werror','-Wno-unused-function','-Wno-unused-variable',
  '-fsanitize=address,undefined','-DPIONEER_Q38_HYBRID_CANDIDATE_UPLOAD_CLIENT=1',
  here/'pioneer_q38_hybrid_tracked_session_test.cpp','-o',candidate)
 run(candidate);run(candidate,'--seal-failure');run(candidate,'--bad-seal')
 # Cross-build both explicitly tagged, non-interchangeable client variants.
 run(sys.executable,root/'build_q38_hybrid_upload_diagnostic.py','--tracked-compile-only',sdk,out/'build')
 run(sys.executable,root/'build_q38_hybrid_upload_diagnostic.py','--candidate-build-only',sdk,out/'candidate-build')
 source='#define PIONEER_Q38_HYBRID_UPLOAD_DIAGNOSTIC 1\n#include "'+str(root/'PioneerQ38HybridUploadSession.h')+'"\n'
 result=subprocess.run([compiler,'-std=c++17','-DPIONEER_Q38_HYBRID_TRACKED_UPLOAD_CLIENT=0','-E','-x','c++','-'],
  input=source,capture_output=True,text=True)
 assert result.returncode and 'Hybrid tracked upload client requires its distinct diagnostic selection' in result.stderr
 for flags in (['-DPIONEER_Q38_HYBRID_CANDIDATE_UPLOAD_CLIENT=1'],
               ['-DPIONEER_Q38_HYBRID_TRACKED_UPLOAD_CLIENT=1','-DPIONEER_Q38_HYBRID_CANDIDATE_UPLOAD_CLIENT=0']):
  result=subprocess.run([compiler,'-std=c++17',*flags,'-E','-x','c++','-'],
   input=source,capture_output=True,text=True)
  assert result.returncode and 'Hybrid candidate uploader requires the distinct tracked client' in result.stderr
 result=subprocess.run([sys.executable,str(root/'build_q38_hybrid_upload_diagnostic.py'),
  '--candidate-build-only','--tracked-compile-only',str(sdk),str(out/'invalid')],capture_output=True,text=True)
 assert result.returncode and not (out/'invalid').exists()
 print('PASS tracked model-loader handshake and Haiku cross-build; no device, model inference or speed result')
