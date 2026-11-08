#!/usr/bin/env python3
"""Build the filtered wrapper over the frozen all-BFP8 writer; never run it."""
import ast
import hashlib
import os
from pathlib import Path
import re
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
root = here.parent.parent
frozen_dir = here / 'frozen_bfp8'
old_builder = here / 'build_qwen38_native_weight_cache.py'
# The original writer fingerprints its cache reader as well as the codec.
# The current reader has since grown a safe existing-only API, so preserve and
# compile the actual pinned old cache API/implementation for THIS writer.
tree = ast.parse(old_builder.read_text())
assignments = [node for node in tree.body if isinstance(node, ast.Assign)
               and len(node.targets) == 1 and isinstance(node.targets[0], ast.Name)
               and node.targets[0].id == 'inputs']
if len(assignments) != 1:
    raise RuntimeError('unexpected frozen writer input declaration')
inputs = ast.literal_eval(assignments[0].value)
if not isinstance(inputs, tuple) or len(inputs) != 11 or 'PioneerModelWeightCache.cpp' not in inputs:
    raise RuntimeError('unexpected frozen writer input list')
inputs += ('PioneerQ38NativeBfp8WeightTiles.h',)
identity = hashlib.sha256(b'all-bfp8\0')
for name in inputs:
    source = frozen_dir / name if name in ('PioneerModelWeightCache.h', 'PioneerModelWeightCache.cpp') else here / name
    identity.update(name.encode() + b'\0' + source.read_bytes() + b'\0')
fingerprint = identity.hexdigest()
if fingerprint != '7db613bfdde463be7f933f25123a00dbe39928cbd8a070aa2481c6cb936753d0':
    raise RuntimeError('frozen all-BFP8 writer changed; do not reuse its cache recipe')
if any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ):
    raise RuntimeError('unexpected Pioneer environment override')
output.mkdir(mode=0o700)
binary = output / 'pioneer-q38-hybrid-bfp8-prepare'
command = [os.environ.get('CXX', 'c++'), '-std=c++17', '-O2', '-g', '-Wall', '-Wextra', '-Werror',
           '-ffp-contract=off', '-fstack-usage', '-ffile-prefix-map=' + str(root) + '=PioneerSource',
           '-fdebug-prefix-map=' + str(output) + '=NativeWeightCacheBuild', '-I' + str(here),
           '-DPIONEER_Q38_NATIVE_CONVERSION_SHA256="' + fingerprint + '"',
           '-DPIONEER_Q38_ALL_BFP8_CACHE=1', '-DPIONEER_Q38_HYBRID_BFP8_PREPARER=1']
if qgw8:
    command.append('-DPIONEER_Q38_HYBRID_BFP8_WINDOW11=1')
if len(arguments) == 3:
    build = Path(arguments[2])
    if not build.is_absolute() or not build.is_dir():
        raise RuntimeError('invalid Haiku build')
    command[0] = str(build / 'cross-tools-riscv64/bin/riscv64-unknown-haiku-g++')
    sdk = build / 'objects/haiku/riscv64/packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop'
    release = build / 'objects/haiku/riscv64/release'
    command += ['-I' + str(sdk / 'headers'), '-idirafter', str(sdk / 'headers/posix')]
    for path in [sdk / 'headers/os', *sorted((sdk / 'headers/os').iterdir())]:
        if path.is_dir():
            command += ['-isystem', str(path)]
    command += ['-I' + str(root / 'headers/private/shared'), '-I' + str(root / 'headers/private/libroot'),
                '-B' + str(release / 'system/glue'), '-L' + str(release / 'system/libroot'), '-L' + str(release / 'kits')]
    command += ['-L' + str(path) for path in sorted((build / 'build_packages').glob('gcc_syslibs-*/lib'))]
command += [str(frozen_dir / 'pioneer_q38_hybrid_bfp8_frozen.cpp'), str(frozen_dir / 'PioneerModelWeightCache.cpp')]
if len(arguments) == 3:
    command += [str(release / 'kits/shared/libshared.a'), '-lbe']
command += ['-o', str(binary)]
subprocess.run(command, check=True, cwd=output)
(output / 'build-argv.txt').write_text('\t'.join(command) + '\n')
preprocess = subprocess.run([*command[:command.index('-o')], '-E'], cwd=output,
                            stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, check=True)
if len(re.findall(r'using\s+Tiles\s*=\s*PioneerQ38HybridBfp8Tiles\s*;', preprocess.stdout)) != 1:
    raise RuntimeError('frozen writer did not bind exactly one filtered tile adapter')
if 'OpenExistingDirectory' in preprocess.stdout:
    raise RuntimeError('frozen writer accidentally included the newer cache API')
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
