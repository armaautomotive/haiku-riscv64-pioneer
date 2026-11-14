#!/usr/bin/env python3
"""Cross-build an isolated source preflight; never run it or touch a device."""
import hashlib
import os
from pathlib import Path
import subprocess
import sys

if not __debug__ or os.environ.get('PYTHONOPTIMIZE') or len(sys.argv)!=4 or sys.argv[3] not in ('--small-alpha','--small-beta') or any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ):
    raise SystemExit('usage: ABS_DIAGNOSTIC_BUILD ABS_NEW_OUTPUT --small-alpha|--small-beta')
build,output=map(Path,sys.argv[1:3]);alpha=sys.argv[3]=='--small-alpha';kind='ALPHA' if alpha else 'BETA'
assert build.is_absolute() and build.is_dir() and not build.is_symlink()
assert output.is_absolute() and not output.exists() and not output.is_symlink()
here=Path(__file__).resolve().parent;source=str(here/'pioneer_native_model_band_diagnostic.cpp')
matches=[line.split('\t') for line in (build/'build-argv.txt').read_text().splitlines() if source in line]
assert len(matches)==1;args=matches[0]
diag='-DPIONEER_NATIVE_SMALL_'+kind+'_BAND_DIAGNOSTIC=1';src='-DPIONEER_NATIVE_SMALL_'+kind+'_BAND_SOURCE=1'
assert args[0].endswith('/cross-tools-riscv64/bin/riscv64-unknown-haiku-g++') and args.count(diag)==args.count(src)==args.count(source)==1
assert args.count('-o')==1 and args.index('-o')==len(args)-2 and Path(args[-1]).parent==build
metadata=[Path(a[2:])/'PioneerNativeBfpModelBandMetadata.h' for a in args if a.startswith('-I') and (Path(a[2:])/'PioneerNativeBfpModelBandMetadata.h').is_file()]
assert len(metadata)==1
pin='630caa504d3b3fc96b2aa38ec090954c1cbb470b2baa59d84957af4e7d4915e1' if alpha else '70940f99172a098a48ab18b978e9183a6ade1e1543f16fb06f42fa6d993421e4'
assert hashlib.sha256(metadata[0].read_bytes()).hexdigest()==pin
args.remove(diag);args[args.index(source)]=str(here/'pioneer_qgwd_dense_source_preflight.cpp')
output.mkdir(mode=0o700);binary=output/('pioneer-small-alpha-preflight' if alpha else 'pioneer-small-beta-preflight');args[-1]=str(binary)
subprocess.run(args,check=True,cwd=build)
assert hashlib.sha256(metadata[0].read_bytes()).hexdigest()==pin
print('PREFLIGHT_SHA256',hashlib.sha256(binary.read_bytes()).hexdigest())
print('BUILD ONLY; PRIVATE MODEL-DERIVED METADATA EMBEDDED; NO MODEL OR DEVICE ACCESS')
