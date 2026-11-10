#!/usr/bin/env python3
"""Cross-build the existing read-only QGWD cache checker; never run it."""
import hashlib
import os
from pathlib import Path
import subprocess
import sys

if len(sys.argv) != 3 or any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ):
    raise SystemExit('usage: absolute-writer-build-dir absolute-new-output-dir')
writer, output = map(Path, sys.argv[1:])
if not writer.is_absolute() or not output.is_absolute() or not writer.is_dir() or output.exists() or output.is_symlink():
    raise SystemExit('writer must exist; output must be a new absolute directory')
here = Path(__file__).resolve().parent
arguments = (writer / 'build-argv.txt').read_text().strip().split('\t')
if not arguments or not arguments[0].endswith('/cross-tools-riscv64/bin/riscv64-unknown-haiku-g++'):
    raise SystemExit('expected a Haiku RISC-V converter toolchain')
old_sources = [str(here/'frozen_bfp8/pioneer_q38_hybrid_bfp8_frozen.cpp'),
               str(here/'frozen_bfp8/PioneerModelWeightCache.cpp')]
if any(arguments.count(source) != 1 for source in old_sources) or arguments.count('-o') != 1:
    raise SystemExit('unexpected converter build command')
if any(arguments.count(flag) != 1 for flag in ('-DPIONEER_Q38_ALL_BFP8_CACHE=1',
        '-DPIONEER_Q38_HYBRID_BFP8_PREPARER=1', '-DPIONEER_Q38_DENSE_Q8_BFP8_PREPARER=1')):
    raise SystemExit('expected the distinct dense converter build')
out_index = arguments.index('-o')
if out_index != len(arguments)-2 or Path(arguments[-1]).parent != writer:
    raise SystemExit('unexpected converter output')
output.mkdir(mode=0o700)
binary = output/'pioneer-qgwd-dense-cache-check'
filtered = [arg for arg in arguments[:out_index] if arg not in old_sources and not arg.startswith((
    '-DPIONEER_Q38_NATIVE_CONVERSION_SHA256=', '-DPIONEER_Q38_ALL_BFP8_CACHE=',
    '-DPIONEER_Q38_HYBRID_BFP8_PREPARER=', '-DPIONEER_Q38_DENSE_Q8_BFP8_PREPARER='))]
# Insert source files before static libraries; no driver or model execution.
library_index = filtered.index('-lbe')-1
filtered[library_index:library_index] = ['-DPIONEER_Q38_REAL_DENSE_Q8_CACHE_CHECK=1',
    '-Wno-misleading-indentation', str(here/'tests/pioneer_q38_dense_q8_real_cache_check.cpp'),
    str(here/'PioneerModelWeightCache.cpp')]
subprocess.run([*filtered, '-o', str(binary)], check=True, cwd=output)
print('CHECKER_SHA256', hashlib.sha256(binary.read_bytes()).hexdigest(), flush=True)
print('BUILD ONLY: no model, cache or device access', flush=True)
