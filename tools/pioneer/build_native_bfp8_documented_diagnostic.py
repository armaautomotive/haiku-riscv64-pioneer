#!/usr/bin/env python3
"""Build NEW strict documented-rounding client+UNCHANGED driver;NO install/GPU."""
import argparse
import hashlib
import os
import pathlib
import subprocess
import sys
if not __debug__ or os.environ.get('PYTHONOPTIMIZE'):
    raise SystemExit('Assertions required')
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('haiku_build', type=pathlib.Path)
parser.add_argument('output', type=pathlib.Path)
args = parser.parse_args()
assert args.output.is_absolute() and not args.output.exists() and not args.output.is_symlink()
assert args.haiku_build.is_absolute() and args.haiku_build.is_dir()
assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
here = pathlib.Path(__file__).resolve().parent
record = here / 'benchmarks/qwen38-native-bfp8-observe-20261104/fixed8-returned.stderr'
assert hashlib.sha256(record.read_bytes()).hexdigest() == '66112229d3b318d6e79864ecf6d45ce3007a32ae44b7b6c939ce7b05c6ccdf36'
args.output.mkdir(mode=0o700)
legacy = args.output / 'unchanged-driver'
subprocess.run([sys.executable, str(here / 'build_native_bfp_diagnostic.py'), str(args.haiku_build), str(legacy)], check=True)
for name, digest in (
    ('tenstorrent_pci_probe-native-bfp78', 'e79e070b9e4fe434b868898748834e0cd89b03bcb250d9c1694b4d0b890f46ac'),
    ('pioneer-native-bfp-diagnostic', '428e7a15ad97501eb711dc6a2e59f30c68d14c1b022acc32b9b753ce793aefb4')):
    assert hashlib.sha256((legacy / name).read_bytes()).hexdigest() == digest
build = args.haiku_build
sdk = build / 'objects/haiku/riscv64/packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop'
release = build / 'objects/haiku/riscv64/release'
command = [str(build / 'cross-tools-riscv64/bin/riscv64-unknown-haiku-g++'), '-std=c++17', '-O2', '-ffp-contract=off',
           '-Wall', '-Wextra', '-Werror', '-I' + str(sdk / 'headers'), '-idirafter', str(sdk / 'headers/posix')]
for path in [sdk / 'headers/os', *sorted((sdk / 'headers/os').iterdir())]:
    if path.is_dir():
        command += ['-isystem', str(path)]
command += ['-B' + str(release / 'system/glue'), '-L' + str(release / 'system/libroot')]
for path in sorted((build / 'build_packages').glob('gcc_syslibs-*/lib')):
    command += ['-L' + str(path)]
binary = args.output / 'pioneer-native-bfp8-documented-diagnostic'
command += [str(here / 'pioneer_native_bfp8_documented_diagnostic.cpp'), '-o', str(binary)]
subprocess.run(command, check=True, cwd=args.output)
(args.output / 'client-build-argv.txt').write_text('\n'.join(command) + '\n')
print(hashlib.sha256(binary.read_bytes()).hexdigest(), binary.name)
print('BUILD_ONLY separate strict fixed-BFP8 documented-rounding diagnostic;NO install/device/Work/full-K/model-quality/speed authority')
