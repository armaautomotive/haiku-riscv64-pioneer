#!/usr/bin/env python3
"""Cross-build the separate hybrid full candidate diagnostic; never run it."""
import hashlib
from pathlib import Path
import runpy
import subprocess
import sys

args=sys.argv[1:];fixed='--fixed-126' in args;matrix='--matrix-127' in args
if matrix:
    assert args.count('--matrix-127')==1 and not fixed;args.remove('--matrix-127')
if fixed:
    assert args.count('--fixed-126')==1;args.remove('--fixed-126')
if not __debug__ or len(args)!=2:
    raise SystemExit('use [--fixed-126|--matrix-127] ABSOLUTE_HAIKU_BUILD NEW_ABSOLUTE_OUTPUT')
sdk,out=map(Path,args);here=Path(__file__).resolve().parent
saved=sys.argv
try:
    sys.argv=[str(here/'build_q38_hybrid_upload_diagnostic.py'),'--candidate-build-only',*(['--matrix-127'] if matrix else ['--fixed-126'] if fixed else []),str(sdk),str(out)]
    base=runpy.run_path(sys.argv[0],run_name='__main__')
finally:
    sys.argv=saved
cmd=list(map(str,base['cmd']));old=str(here/'pioneer_q38_hybrid_upload_diagnostic.cpp')
binary=out/'pioneer-q38-hybrid-full-diagnostic'
if cmd.count(old)!=1 or cmd.count(str(base['binary']))!=1:
    raise RuntimeError('unexpected candidate uploader build command')
cmd[cmd.index(old)]=str(here/'pioneer_q38_hybrid_full_diagnostic.cpp')
cmd[cmd.index(str(base['binary']))]=str(binary)
cmd.insert(1,'-DPIONEER_Q38_HYBRID_FULL_CLIENT=1')
subprocess.run(cmd,check=True,cwd=out)
print('HYBRID_FULL_DIAGNOSTIC_SHA256',hashlib.sha256(binary.read_bytes()).hexdigest())
print('BUILD ONLY: candidate not installed, not run, not numerically qualified and not speed-tested')
