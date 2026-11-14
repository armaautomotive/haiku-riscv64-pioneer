#!/usr/bin/env python3
"""Cross-link one fixed small-recurrent diagnostic client; never run/install."""
import hashlib
import os
from pathlib import Path
import stat
import subprocess
import sys

if not __debug__ or os.environ.get('PYTHONOPTIMIZE') or len(sys.argv)!=5 or sys.argv[4] not in ('--small-alpha','--small-beta') or any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ):
    raise SystemExit('usage: ABS_RECURRENT_BUILD ABS_PRIVATE_EXPORT ABS_NEW_OUTPUT --small-alpha|--small-beta')
build, private, output=map(Path,sys.argv[1:4]);alpha=sys.argv[4]=='--small-alpha'
assert all(p.is_absolute() for p in (build,private,output))
assert build.is_dir() and not build.is_symlink() and not private.is_symlink() and not output.exists() and not output.is_symlink()
info=private.stat();assert stat.S_ISDIR(info.st_mode) and info.st_uid==os.geteuid() and stat.S_IMODE(info.st_mode)==0o700
metadata=private/'PioneerNativeBfpModelBandMetadata.h';info=metadata.lstat()
assert stat.S_ISREG(info.st_mode) and info.st_nlink==1 and info.st_uid==os.geteuid() and stat.S_IMODE(info.st_mode)==0o600
pin='630caa504d3b3fc96b2aa38ec090954c1cbb470b2baa59d84957af4e7d4915e1' if alpha else '70940f99172a098a48ab18b978e9183a6ade1e1543f16fb06f42fa6d993421e4'
assert hashlib.sha256(metadata.read_bytes()).hexdigest()==pin
here=Path(__file__).resolve().parent;source=str(here/'pioneer_native_model_band_diagnostic.cpp')
lines=(build/'build-argv.txt').read_text().splitlines();matches=[line.split('\t') for line in lines if source in line]
assert len(matches)==1;args=matches[0]
assert args[0].endswith('/cross-tools-riscv64/bin/riscv64-unknown-haiku-g++') and args.count(source)==1
old_diag='-DPIONEER_NATIVE_RECURRENT_BAND_DIAGNOSTIC=1';old_source='-DPIONEER_NATIVE_RECURRENT_BAND_SOURCE=1'
assert args.count(old_diag)==args.count(old_source)==1 and args.count('-o')==1 and args.index('-o')==len(args)-2
assert Path(args[-1]).parent==build
old_includes=[i for i,a in enumerate(args) if a.startswith('-I') and (Path(a[2:])/'PioneerNativeBfpModelBandMetadata.h').is_file()]
assert len(old_includes)==1
assert hashlib.sha256((Path(args[old_includes[0]][2:])/'PioneerNativeBfpModelBandMetadata.h').read_bytes()).hexdigest()=='a5fadca5d6baeaa921472989cf038dd4aa2a90a666f4c7b3262c8ee141841ff2'
kind='ALPHA' if alpha else 'BETA'
args[args.index(old_diag)]='-DPIONEER_NATIVE_SMALL_'+kind+'_BAND_DIAGNOSTIC=1'
args[args.index(old_source)]='-DPIONEER_NATIVE_SMALL_'+kind+'_BAND_SOURCE=1'
args[old_includes[0]]='-I'+str(private)
output.mkdir(mode=0o700);binary=output/('pioneer-small-alpha-client' if alpha else 'pioneer-small-beta-client');args[-1]=str(binary)
subprocess.run(args,check=True,cwd=build)
assert hashlib.sha256(metadata.read_bytes()).hexdigest()==pin
print('CLIENT_SHA256',hashlib.sha256(binary.read_bytes()).hexdigest())
print('BUILD ONLY; PRIVATE MODEL-DERIVED METADATA EMBEDDED; NO MODEL, DRIVER OR DEVICE ACCESS')
