#!/usr/bin/env python3
"""Cross-build the recurrent CPU-only source preflight; never run or install."""
import hashlib
import os
from pathlib import Path
import subprocess
import sys

if len(sys.argv)!=3 or any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ):
    raise SystemExit('usage: ABSOLUTE_RECURRENT_BUILD NEW_ABSOLUTE_OUTPUT')
build, output = map(Path,sys.argv[1:]);here=Path(__file__).resolve().parent
assert build.is_absolute() and build.is_dir() and not build.is_symlink()
assert output.is_absolute() and not output.exists() and not output.is_symlink()
lines=(build/'build-argv.txt').read_text().splitlines()
matches=[line.split('\t') for line in lines if str(here/'pioneer_native_model_band_diagnostic.cpp') in line]
assert len(matches)==1
args=matches[0]
assert args[0].endswith('/cross-tools-riscv64/bin/riscv64-unknown-haiku-g++')
assert args.count('-DPIONEER_NATIVE_RECURRENT_BAND_DIAGNOSTIC=1')==1
assert args.count('-DPIONEER_NATIVE_RECURRENT_BAND_SOURCE=1')==1
assert args.count('-o')==1 and args.index('-o')==len(args)-2 and Path(args[-1]).parent==build
args.remove('-DPIONEER_NATIVE_RECURRENT_BAND_DIAGNOSTIC=1')
args[args.index(str(here/'pioneer_native_model_band_diagnostic.cpp'))]=str(here/'pioneer_qgwd_dense_source_preflight.cpp')
output.mkdir(mode=0o700);binary=output/'pioneer-recurrent-source-preflight'
args[-1]=str(binary)
subprocess.run(args,check=True,cwd=build)
print('PREFLIGHT_SHA256',hashlib.sha256(binary.read_bytes()).hexdigest())
print('BUILD ONLY; NO MODEL OR DEVICE ACCESS')
