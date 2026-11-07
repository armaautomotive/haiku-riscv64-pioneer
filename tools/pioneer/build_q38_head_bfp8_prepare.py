#!/usr/bin/env python3
"""Build an isolated CPU-only output-head cache preparer for Haiku."""
import hashlib
import os
from pathlib import Path
import runpy
import subprocess
import sys

assert __debug__ and len(sys.argv) == 3
assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
sdk, out = map(Path, sys.argv[1:])
here = Path(__file__).resolve().parent
assert sdk.is_absolute() and sdk.is_dir() and out.is_absolute() and not out.exists() and not out.is_symlink()
out.mkdir(mode=0o700)
saved = sys.argv[:]
try:
    sys.argv = [str(here / 'build_bfp4_fragment_probe.py'), str(sdk), str(out / 'fingerprint-build')]
    original = runpy.run_path(sys.argv[0], run_name='__main__')
finally:
    sys.argv = saved

# Include the existing compiler/decoder fingerprint as well as the head codec
# and its page layout. A future conversion change cannot silently reuse an old
# cache entry. This is a conversion identity, not a model authentication hash.
identity = hashlib.sha256(b'pioneer-q38-head-bfp8-converter-v1\0')
identity.update(original['fingerprint'].encode('ascii'))
for name in ('PioneerQ38HeadBfp8FragmentSource.h', 'PioneerQ38HeadBfp8FragmentCache.h',
             'PioneerQ38HeadBlockFloatDiagnostic.h', 'PioneerBlackholeBlockFloat.h',
             'PioneerQwen38Gguf.h', 'PioneerQwen38PackedPlan.h', 'TenstorrentGgufQuant.h', 'TenstorrentQ4K.h'):
    data = (here / name).read_bytes()
    identity.update(name.encode('ascii') + b'\0')
    identity.update(len(data).to_bytes(8, 'little'))
    identity.update(data)
fingerprint = identity.hexdigest()
cmd = list(map(str, original['cacheCmd']))
old = str(here / 'pioneer_q38_bfp4_cache_probe.cpp')
assert cmd.count(old) == 1
cmd[cmd.index(old)] = str(here / 'pioneer_q38_head_bfp8_prepare.cpp')
old = str(original['cacheProbe'])
assert cmd.count(old) == 1
binary = out / 'pioneer-q38-head-bfp8-prepare'
cmd[cmd.index(old)] = str(binary)
old = '-DPIONEER_BFP4_IMPLEMENTATION_SHA256="' + original['fingerprint'] + '"'
assert cmd.count(old) == 1
cmd[cmd.index(old)] = '-DPIONEER_HEAD_BFP8_IMPLEMENTATION_SHA256="' + fingerprint + '"'
cmd.insert(1, '-DPIONEER_Q38_HEAD_BLOCK_FLOAT_DIAGNOSTIC=1')
# GCC diagnoses the project's compact, semicolon-separated guard style; keep
# every other warning fatal, as in the existing probe build.
cmd.insert(1, '-Wno-misleading-indentation')
subprocess.run(cmd, check=True, cwd=out)
(out / 'build-argv.txt').write_text('\t'.join(cmd) + '\n')
print('HEAD_BFP8_CONVERTER_FINGERPRINT', fingerprint)
print('BINARY_SHA256', hashlib.sha256(binary.read_bytes()).hexdigest())
print('BUILD ONLY: no model reads, cache writes, accelerator or inference')
