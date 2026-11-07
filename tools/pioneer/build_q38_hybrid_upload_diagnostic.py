#!/usr/bin/env python3
"""Cross-build the isolated hybrid upload/release diagnostic, never run it."""
import hashlib
from pathlib import Path
import runpy
import subprocess
import sys

args=sys.argv[1:];candidate='--candidate-build-only' in args;tracked='--tracked-compile-only' in args;fixed='--fixed-126' in args;matrix='--matrix-127' in args
if matrix:
    assert args.count('--matrix-127')==1 and candidate and not fixed;args.remove('--matrix-127')
if fixed:
    assert args.count('--fixed-126')==1 and candidate;args.remove('--fixed-126')
if candidate:
    assert args.count('--candidate-build-only')==1 and not tracked;args.remove('--candidate-build-only')
if tracked:
    assert args.count('--tracked-compile-only')==1;args.remove('--tracked-compile-only')
tracked=tracked or candidate
if not __debug__ or len(args)!=2:
    raise SystemExit('use [--tracked-compile-only|--candidate-build-only] ABSOLUTE_HAIKU_BUILD NEW_ABSOLUTE_OUTPUT_DIRECTORY')
sdk,out=map(Path,args);here=Path(__file__).resolve().parent
saved=sys.argv
try:
    sys.argv=[str(here/'build_q38_hybrid_cache_preflight.py'),str(sdk),str(out)]
    base=runpy.run_path(sys.argv[0],run_name='__main__')
finally:
    sys.argv=saved
cmd=list(map(str,base['cmd']))
old=str(here/'pioneer_q38_hybrid_cache_preflight.cpp');binary=out/'pioneer-q38-hybrid-upload-diagnostic'
if cmd.count(old)!=1 or cmd.count(str(base['binary']))!=1:
    raise RuntimeError('unexpected preflight build command')
cmd[cmd.index(old)]=str(here/'pioneer_q38_hybrid_upload_diagnostic.cpp')
cmd[cmd.index(str(base['binary']))]=str(binary)
cmd.insert(1,'-DPIONEER_Q38_HYBRID_UPLOAD_DIAGNOSTIC=1')
if tracked:cmd.insert(1,'-DPIONEER_Q38_HYBRID_TRACKED_UPLOAD_CLIENT=1')
if candidate:cmd.insert(1,'-DPIONEER_Q38_HYBRID_CANDIDATE_UPLOAD_CLIENT=1')
if fixed:cmd.insert(1,'-DPIONEER_Q38_HYBRID_FIXED_126=1')
if matrix:cmd.insert(1,'-DPIONEER_Q38_HYBRID_MATRIX_CLIENT_127=1')
subprocess.run(cmd,check=True,cwd=out)
print('HYBRID_UPLOAD_DIAGNOSTIC_SHA256',hashlib.sha256(binary.read_bytes()).hexdigest())
print('BUILD ONLY; NO DEVICE ACCESS, UPLOAD OR INFERENCE'+(('; CANDIDATE TAG127, NOT QUALIFIED' if matrix else '; CANDIDATE TAG126, NOT QUALIFIED' if fixed else '; CANDIDATE TAG125, NOT QUALIFIED') if candidate else '; TRACKED CLIENT REQUIRES NON-LOADABLE TAG' if tracked else ''))
