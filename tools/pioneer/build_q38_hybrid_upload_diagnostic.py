#!/usr/bin/env python3
"""Cross-build the isolated hybrid upload/release diagnostic, never run it."""
import hashlib
from pathlib import Path
import runpy
import subprocess
import sys

if not __debug__ or len(sys.argv)!=3:
    raise SystemExit('use ABSOLUTE_HAIKU_BUILD NEW_ABSOLUTE_OUTPUT_DIRECTORY')
sdk,out=map(Path,sys.argv[1:]);here=Path(__file__).resolve().parent
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
subprocess.run(cmd,check=True,cwd=out)
print('HYBRID_UPLOAD_DIAGNOSTIC_SHA256',hashlib.sha256(binary.read_bytes()).hexdigest())
print('BUILD ONLY; NO DEVICE ACCESS, UPLOAD OR INFERENCE')
