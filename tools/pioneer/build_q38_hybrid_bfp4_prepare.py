#!/usr/bin/env python3
"""Build CPU-only hybrid fragment preparer with the existing exact fingerprint."""
import hashlib
import os
from pathlib import Path
import runpy
import subprocess
import sys

assert __debug__ and len(sys.argv)==3 and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
sdk,out=map(Path,sys.argv[1:]);here=Path(__file__).resolve().parent
assert sdk.is_absolute() and sdk.is_dir() and out.is_absolute() and not out.exists() and not out.is_symlink()
out.mkdir(mode=0o700);saved=sys.argv[:]
try:
 sys.argv=[str(here/'build_bfp4_fragment_probe.py'),str(sdk),str(out/'fingerprint-build')]
 original=runpy.run_path(sys.argv[0],run_name='__main__')
finally:sys.argv=saved
cmd=list(map(str,original['cacheCmd']))
old=str(here/'pioneer_q38_bfp4_cache_probe.cpp');assert cmd.count(old)==1
cmd[cmd.index(old)]=str(here/'pioneer_q38_hybrid_bfp4_prepare.cpp')
old=str(original['cacheProbe']);assert cmd.count(old)==1
binary=out/'pioneer-q38-hybrid-bfp4-prepare';cmd[cmd.index(old)]=str(binary)
subprocess.run(cmd,check=True,cwd=out)
(out/'build-argv.txt').write_text('\t'.join(cmd)+'\n')
print('BFP4_CONVERTER_FINGERPRINT',original['fingerprint'])
print('BINARY_SHA256',hashlib.sha256(binary.read_bytes()).hexdigest())
print('BUILD ONLY: no model reads, cache writes, accelerator or inference')
