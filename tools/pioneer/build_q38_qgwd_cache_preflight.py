#!/usr/bin/env python3
"""Build isolated QGWD four-cache CPU preflight using a checked QGW8 build."""
import hashlib
import os
from pathlib import Path
import subprocess
import sys
if len(sys.argv)!=3 or any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ):
    raise SystemExit('usage: absolute-qgw8-build-dir absolute-new-output-dir')
parent,out=map(Path,sys.argv[1:])
if not parent.is_absolute() or not parent.is_dir() or not out.is_absolute() or out.exists() or out.is_symlink():
    raise SystemExit('invalid input or output path')
here=Path(__file__).resolve().parent
args=(parent/'qgw8-preflight-build-argv.txt').read_text().strip().split('\t')
old=str(here/'pioneer_q38_qgw8_cache_preflight.cpp')
required=['-DPIONEER_BFP4_IMPLEMENTATION_SHA256="5dc4ffb1257bf99a8b38f7139e5fe95d04e42f11d63beda382d78aeacb1dc45c"',
 '-DPIONEER_HEAD_BFP8_IMPLEMENTATION_SHA256="e0e45dce6f1d95124aaeb956e2ee4076aff5b11d35d8079e205054cf33cf316f"',
 '-DPIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION=1','-DPIONEER_Q38_BFP4_PREPARATION_DIAGNOSTIC=1']
if (not args or not args[0].endswith('/cross-tools-riscv64/bin/riscv64-unknown-haiku-g++')
 or args.count(old)!=1 or args.count('-o')!=1 or args.index('-o')!=len(args)-2
 or any(args.count(flag)!=1 for flag in required)):
    raise SystemExit('unexpected QGW8 preflight build command')
out.mkdir(mode=0o700);binary=out/'pioneer-q38-qgwd-cache-preflight'
args[args.index(old)]=str(here/'pioneer_q38_qgwd_cache_preflight.cpp');args[-1]=str(binary)
args[1:1]=['-DPIONEER_Q38_DENSE_Q8_VERIFIED_MODEL_INSPECTION=1','-DPIONEER_Q38_DENSE_Q8_MATRIX_PLAN_INSPECTION=1']
subprocess.run(args,check=True,cwd=parent)
print('QGWD_PREFLIGHT_SHA256',hashlib.sha256(binary.read_bytes()).hexdigest(),flush=True)
print('BUILD ONLY: no model, cache, device or inference accessed',flush=True)
