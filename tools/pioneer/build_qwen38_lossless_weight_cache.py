#!/usr/bin/env python3
"""Build lossless weight-cache preparation command; no install/model/GPU access."""
import argparse
import hashlib
import os
import pathlib
import subprocess
from qwen38_lossless_cache_recipe import fingerprint

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('output', type=pathlib.Path)
parser.add_argument('--haiku-build', type=pathlib.Path)
args = parser.parse_args()
here = pathlib.Path(__file__).resolve().parent
root = here.parent.parent
assert args.output.is_absolute() and not args.output.exists() and not args.output.is_symlink()
assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
args.output.mkdir(mode=0o700)
source_sha = fingerprint()
command = [os.environ.get('CXX', 'c++'), '-std=c++17', '-O2', '-g', '-Wall', '-Wextra', '-Werror',
           '-ffp-contract=off', '-DPIONEER_Q38_LOSSLESS_CONVERSION_SHA256="' + source_sha + '"']
if args.haiku_build:
    build = args.haiku_build
    assert build.is_absolute() and build.is_dir()
    command[0] = str(build / 'cross-tools-riscv64/bin/riscv64-unknown-haiku-g++')
    sdk = build / 'objects/haiku/riscv64/packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop'
    release = build / 'objects/haiku/riscv64/release'
    command += ['-I' + str(sdk / 'headers'), '-idirafter', str(sdk / 'headers/posix')]
    for path in [sdk / 'headers/os', *sorted((sdk / 'headers/os').iterdir())]:
        if path.is_dir():
            command += ['-isystem', str(path)]
    command += ['-I' + str(root / 'headers/private/shared'), '-I' + str(root / 'headers/private/libroot'),
                '-B' + str(release / 'system/glue'), '-L' + str(release / 'system/libroot'), '-L' + str(release / 'kits')]
    for path in sorted((build / 'build_packages').glob('gcc_syslibs-*/lib')):
        command += ['-L' + str(path)]
command += [str(here / 'pioneer_q38_lossless_weight_cache.cpp'), str(here / 'PioneerModelWeightCache.cpp')]
if args.haiku_build:
    command += [str(release / 'kits/shared/libshared.a'), '-lbe']
command += ['-o', str(args.output / 'pioneer-q38-lossless-weight-cache')]
subprocess.run(command, check=True, cwd=args.output)
(args.output / 'conversion-source-sha256.txt').write_text(source_sha + '\n')
(args.output / 'build-argv.txt').write_text('\n'.join(command) + '\n')
print('CONVERSION_SOURCE_SHA256', source_sha)
print('BINARY_SHA256', hashlib.sha256((args.output / 'pioneer-q38-lossless-weight-cache').read_bytes()).hexdigest())
