#!/usr/bin/env python3
"""Build CPU-only cache/converter;never installs,opens a card or converts a model."""
import argparse
import hashlib
import os
import pathlib
import subprocess

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('output', type=pathlib.Path)
parser.add_argument('--haiku-build', type=pathlib.Path)
args = parser.parse_args()
here = pathlib.Path(__file__).resolve().parent
root = here.parent.parent
assert args.output.is_absolute() and not args.output.exists() and not args.output.is_symlink()
assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
inputs = ('PioneerBlackholeBlockFloat.h', 'PioneerQ38NativeWeightTiles.h',
          'PioneerQwen38Gguf.h', 'TenstorrentGgufQuant.h', 'TenstorrentQ4K.h',
          'TenstorrentGgufTensor.h', 'PioneerAttentionMath.h',
          'PioneerModelWeightCache.h', 'PioneerModelWeightCache.cpp',
          'pioneer_q38_native_weight_cache.cpp', 'build_qwen38_native_weight_cache.py')
digest = hashlib.sha256()
for name in inputs:
    digest.update(name.encode() + b'\0' + (here / name).read_bytes() + b'\0')
fingerprint = digest.hexdigest()
args.output.mkdir(mode=0o700)
command = [os.environ.get('CXX', 'c++'), '-std=c++17', '-O2', '-g', '-Wall', '-Wextra', '-Werror',
           '-ffp-contract=off', '-fstack-usage', '-ffile-prefix-map=' + str(root) + '=PioneerSource',
           '-fdebug-prefix-map=' + str(args.output) + '=NativeWeightCacheBuild',
           '-DPIONEER_Q38_NATIVE_CONVERSION_SHA256="' + fingerprint + '"']
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
command += [str(here / 'pioneer_q38_native_weight_cache.cpp'), str(here / 'PioneerModelWeightCache.cpp')]
if args.haiku_build:
    command += [str(release / 'kits/shared/libshared.a'), '-lbe']
command += ['-o', str(args.output / 'pioneer-q38-native-weight-cache')]
subprocess.run(command, check=True, cwd=args.output)
(args.output / 'conversion-source-sha256.txt').write_text(fingerprint + '\n')
(args.output / 'build-argv.txt').write_text('\n'.join(command) + '\n')
binary = args.output / 'pioneer-q38-native-weight-cache'
print('CONVERSION_SOURCE_SHA256', fingerprint)
print('BINARY_SHA256', hashlib.sha256(binary.read_bytes()).hexdigest())
print('BUILD_ONLY CPU/userspace converted-weight cache;NO model conversion,installation,GPU,Work or speed proof')
