#!/usr/bin/env python3
"""Build the CPU-only hybrid cache preflight with the frozen BFP4 fingerprint."""
import hashlib
import os
from pathlib import Path
import runpy
import subprocess
import sys

if not __debug__ or len(sys.argv)!=3 or any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ):
    raise SystemExit('use ABSOLUTE_HAIKU_BUILD NEW_ABSOLUTE_OUTPUT')
sdk,out=map(Path,sys.argv[1:]);here=Path(__file__).resolve().parent
if not sdk.is_absolute() or not sdk.is_dir() or not out.is_absolute() or out.exists() or out.is_symlink():
    raise SystemExit('invalid build or output path')
saved=sys.argv
try:
    sys.argv=[str(here/'build_q38_hybrid_bfp4_prepare.py'),str(sdk),str(out)]
    original=runpy.run_path(sys.argv[0],run_name='__main__')
finally:
    sys.argv=saved
if original['original']['fingerprint']!='5dc4ffb1257bf99a8b38f7139e5fe95d04e42f11d63beda382d78aeacb1dc45c':
    raise RuntimeError('BFP4 converter fingerprint changed; existing cache keys cannot be assumed')
cmd=list(map(str,original['cmd']))
old=str(here/'pioneer_q38_hybrid_bfp4_prepare.cpp')
binary=out/'pioneer-q38-hybrid-cache-preflight'
if cmd.count(old)!=1 or cmd.count(str(original['binary']))!=1:
    raise RuntimeError('unexpected preparer build command')
cmd[cmd.index(old)]=str(here/'pioneer_q38_hybrid_cache_preflight.cpp')
cmd[cmd.index(str(original['binary']))]=str(binary)
subprocess.run(cmd,check=True,cwd=out)
print('HYBRID_PREFLIGHT_BINARY_SHA256',hashlib.sha256(binary.read_bytes()).hexdigest())
print('BUILD ONLY: no model, cache, accelerator or inference access')
