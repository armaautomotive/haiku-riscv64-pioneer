#!/usr/bin/env python3
"""Cross-build a private real-Q8 band exporter; never execute it."""
import hashlib
import os
from pathlib import Path
import subprocess
import sys

if len(sys.argv) not in (3, 4) or (len(sys.argv) == 4 and sys.argv[3] != '--recurrent') or any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ):
    raise SystemExit('usage: absolute-writer-build-dir absolute-new-output-dir [--recurrent]')
writer, output = map(Path, sys.argv[1:3])
recurrent = len(sys.argv) == 4
if not writer.is_absolute() or not output.is_absolute() or not writer.is_dir() or output.exists() or output.is_symlink():
    raise SystemExit('writer must exist; output must be a new absolute directory')
here = Path(__file__).resolve().parent
args = (writer/'build-argv.txt').read_text().strip().split('\t')
sources = [str(here/'frozen_bfp8/pioneer_q38_hybrid_bfp8_frozen.cpp'),
           str(here/'frozen_bfp8/PioneerModelWeightCache.cpp')]
if (not args or not args[0].endswith('/cross-tools-riscv64/bin/riscv64-unknown-haiku-g++')
    or any(args.count(source) != 1 for source in sources) or args.count('-o') != 1
    or args.index('-o') != len(args)-2 or Path(args[-1]).parent != writer
    or args.count('-DPIONEER_Q38_DENSE_Q8_BFP8_PREPARER=1') != 1):
    raise SystemExit('unexpected dense Haiku converter build command')
filtered = [arg for arg in args[:-2] if arg not in sources and not arg.startswith((
    '-DPIONEER_Q38_NATIVE_CONVERSION_SHA256=', '-DPIONEER_Q38_ALL_BFP8_CACHE=',
    '-DPIONEER_Q38_HYBRID_BFP8_PREPARER=', '-DPIONEER_Q38_DENSE_Q8_BFP8_PREPARER='))]
library_index = filtered.index('-lbe')-1
filtered[library_index:library_index] = ['-D' + ('PIONEER_NATIVE_RECURRENT_BAND_EXPORT' if recurrent else 'PIONEER_NATIVE_QGWD_DENSE_BAND_EXPORT') + '=1',
    str(here/'pioneer_native_model_band_export.cpp'), str(here/'PioneerModelWeightCache.cpp')]
output.mkdir(mode=0o700)
binary = output/('pioneer-recurrent-band-export' if recurrent else 'pioneer-qgwd-dense-band-export')
subprocess.run([*filtered,'-o',str(binary)],check=True,cwd=writer)
print('EXPORTER_SHA256',hashlib.sha256(binary.read_bytes()).hexdigest(),flush=True)
print('BUILD ONLY: no model, cache, private weights or device access',flush=True)
