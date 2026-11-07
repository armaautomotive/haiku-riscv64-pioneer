#!/usr/bin/env python3
"""Build the filtered wrapper over the frozen all-BFP8 writer; never run it."""
import hashlib
import os
from pathlib import Path
import re
import runpy
import subprocess
import sys

if not __debug__:
    raise SystemExit('Python assertions must be enabled')
arguments = sys.argv[1:]
qgw8 = '--qgw8' in arguments
if arguments.count('--qgw8') > 1:
    raise SystemExit('duplicate --qgw8')
arguments = [arg for arg in arguments if arg != '--qgw8']
if len(arguments) not in (1, 3):
    raise SystemExit('use NEW_ABSOLUTE_OUTPUT [--haiku-build ABSOLUTE_BUILD] [--qgw8]')
if len(arguments) == 3 and arguments[1] != '--haiku-build':
    raise SystemExit('invalid build option')
output = Path(arguments[0])
if not output.is_absolute() or output.exists() or output.is_symlink():
    raise SystemExit('output must be a new absolute directory')
here = Path(__file__).resolve().parent
old_builder = here / 'build_qwen38_native_weight_cache.py'
saved = sys.argv
try:
    sys.argv = [str(old_builder), str(output), '--all-bfp8', *arguments[1:]]
    frozen = runpy.run_path(str(old_builder), run_name='__main__')
finally:
    sys.argv = saved
fingerprint = frozen['fingerprint']
if fingerprint != '7db613bfdde463be7f933f25123a00dbe39928cbd8a070aa2481c6cb936753d0':
    raise RuntimeError('frozen all-BFP8 writer changed; do not reuse its cache recipe')
command = list(frozen['command'])
old_source = str(here / 'pioneer_q38_native_weight_cache.cpp')
old_binary = str(output / 'pioneer-q38-native-bfp8-weight-cache')
if command.count(old_source) != 1 or command.count(old_binary) != 1:
    raise RuntimeError('unexpected frozen builder command')
command[command.index(old_source)] = str(here / 'pioneer_q38_hybrid_bfp8_prepare.cpp')
binary = output / 'pioneer-q38-hybrid-bfp8-prepare'
command[command.index(old_binary)] = str(binary)
command.insert(1, '-DPIONEER_Q38_HYBRID_BFP8_PREPARER=1')
if qgw8:
    command.insert(1, '-DPIONEER_Q38_HYBRID_BFP8_WINDOW11=1')
subprocess.run(command, check=True, cwd=output)
preprocess = subprocess.run([*command[:command.index('-o')], '-E'], cwd=output,
                            stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, check=True)
if len(re.findall(r'using\s+Tiles\s*=\s*PioneerQ38HybridBfp8Tiles\s*;', preprocess.stdout)) != 1:
    raise RuntimeError('frozen writer did not bind exactly one filtered tile adapter')
refusals = []
for name in ('PIONEER_Q38_HYBRID_BFP8_PREPARER', 'PIONEER_Q38_ALL_BFP8_CACHE'):
    refusals += [(['-U'+name], name+' missing'), (['-U'+name, '-D'+name+'=0'], name+' zero')]
if qgw8:
    refusals.append((['-UPIONEER_Q38_HYBRID_BFP8_WINDOW11', '-DPIONEER_Q38_HYBRID_BFP8_WINDOW11=0'], 'QGW8 zero'))
for name in ('_KERNEL_MODE', 'PIONEER_Q38_WORK_INFERENCE', 'TT_PIONEER_Q38_WORK_MODE',
             'PIONEER_Q38_CPU_VALIDATION', 'PIONEER_Q38_FULL_MODEL_CPU_REFERENCE'):
    refusals.append((['-D'+name+'=1'], name))
for flags, label in refusals:
    result = subprocess.run([*command[:command.index('-o')], *flags, '-fsyntax-only'],
                            cwd=output, stdout=subprocess.DEVNULL, stderr=subprocess.PIPE, text=True)
    if result.returncode == 0:
        raise RuntimeError('forbidden role was accepted: '+label)
print('HYBRID_BFP8_FROZEN_WRITER', fingerprint)
print('BINARY_SHA256', hashlib.sha256(binary.read_bytes()).hexdigest())
print('BUILD ONLY: selected%s, no model, cache, device, quality or speed claim' % ('33' if qgw8 else '39'))
