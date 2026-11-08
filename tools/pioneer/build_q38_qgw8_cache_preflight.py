#!/usr/bin/env python3
"""Build the isolated QGW8 three-format CPU cache preflight for Haiku."""
import hashlib
import os
from pathlib import Path
import runpy
import subprocess
import sys

if not __debug__ or len(sys.argv) != 3 or any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ):
    raise SystemExit('use ABSOLUTE_HAIKU_BUILD NEW_ABSOLUTE_OUTPUT')
sdk, out = map(Path, sys.argv[1:])
here = Path(__file__).resolve().parent
if not sdk.is_absolute() or not sdk.is_dir() or not out.is_absolute() or out.exists() or out.is_symlink():
    raise SystemExit('invalid build or output path')
saved = sys.argv[:]
try:
    sys.argv = [str(here / 'build_q38_head_bfp8_prepare.py'), str(sdk), str(out)]
    built = runpy.run_path(sys.argv[0], run_name='__main__')
finally:
    sys.argv = saved

four = built['original']['fingerprint']
if four != '5dc4ffb1257bf99a8b38f7139e5fe95d04e42f11d63beda382d78aeacb1dc45c':
    raise RuntimeError('BFP4 conversion fingerprint differs; do not reuse its cache')
cmd = list(map(str, built['cmd']))
old_source = str(here / 'pioneer_q38_head_bfp8_prepare.cpp')
old_binary = str(built['binary'])
if cmd.count(old_source) != 1 or cmd.count(old_binary) != 1:
    raise RuntimeError('unexpected head preparer build command')
binary = out / 'pioneer-q38-qgw8-cache-preflight'
cmd[cmd.index(old_source)] = str(here / 'pioneer_q38_qgw8_cache_preflight.cpp')
cmd[cmd.index(old_binary)] = str(binary)
cmd[1:1] = ['-DPIONEER_BFP4_IMPLEMENTATION_SHA256="' + four + '"',
            '-DPIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION=1',
            '-DPIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION=1',
            '-DPIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT=1']
subprocess.run(cmd, check=True, cwd=out)
(out / 'qgw8-preflight-build-argv.txt').write_text('\t'.join(cmd) + '\n')
# Instantiate the real QGW8 page source and the explicitly selected span
# packet builder together under the Haiku compiler. This is compile-only;
# it neither creates an owner nor links or opens a device.
probe = list(cmd)
probe[probe.index(str(here / 'pioneer_q38_qgw8_cache_preflight.cpp'))] = str(
    here / 'tests' / 'pioneer_q38_hybrid_head_prepared_actual_compile.cpp')
for item in (str(here / 'PioneerModelWeightCache.cpp'), str(sdk / 'objects/haiku/riscv64/release/kits/shared/libshared.a'), '-lbe'):
    if probe.count(item) != 1:
        raise RuntimeError('unexpected compile-only probe input: ' + item)
    probe.remove(item)
out_index = probe.index('-o')
del probe[out_index:out_index + 2]
probe.append('-fsyntax-only')
subprocess.run(probe, check=True, cwd=out)
session_probe = list(probe)
session_probe[session_probe.index(str(here / 'tests' / 'pioneer_q38_hybrid_head_prepared_actual_compile.cpp'))] = str(
    here / 'tests' / 'pioneer_q38_qgw8_upload_session_compile.cpp')
subprocess.run(session_probe, check=True, cwd=out)
full_session_probe = list(probe)
full_session_probe[full_session_probe.index(str(here / 'tests' / 'pioneer_q38_hybrid_head_prepared_actual_compile.cpp'))] = str(
    here / 'tests' / 'pioneer_q38_qgw8_full_session_compile.cpp')
subprocess.run(full_session_probe, check=True, cwd=out)
print('QGW8_FFN_BFP4_CONVERTER_FINGERPRINT', four)
print('QGW8_HEAD_BFP8_CONVERTER_FINGERPRINT', built['fingerprint'])
print('QGW8_PREFLIGHT_BINARY_SHA256', hashlib.sha256(binary.read_bytes()).hexdigest())
print('BUILD ONLY: preflight binary and real page/span/upload/full-session types; no model, cache, device or inference access')
