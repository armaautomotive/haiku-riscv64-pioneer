#!/usr/bin/env python3
"""Cross-build the offline hybrid startup test with the actual QGW5 selector.

BUILD_ROOT and OUTPUT must be absolute; OUTPUT must not already exist. This
only builds a userspace CPU test. It does not contact or modify a device.
"""
import hashlib
import os
from pathlib import Path
import subprocess
import sys

if not __debug__ or len(sys.argv) != 3 or any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ):
    raise SystemExit('usage: build_pioneer_q38_hybrid_startup_qgw5.py BUILD_ROOT NEW_OUTPUT')
build, output = map(Path, sys.argv[1:])
if not build.is_absolute() or not build.is_dir() or not output.is_absolute() or output.exists() or output.is_symlink():
    raise SystemExit('invalid build root or output')
sdk = build / 'objects/haiku/riscv64/packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop'
release = build / 'objects/haiku/riscv64/release'
cxx = build / 'cross-tools-riscv64/bin/riscv64-unknown-haiku-g++'
includes = ['-I' + str(sdk/'headers'), '-idirafter', str(sdk/'headers/posix')]
for directory in [sdk/'headers/os', *sorted((sdk/'headers/os').iterdir())]:
    if directory.is_dir():
        includes += ['-isystem', str(directory)]
libraries = ['-L' + str(path) for path in sorted((build/'build_packages').glob('gcc_syslibs-*/lib'))]
source = Path(__file__).resolve().parent/'pioneer_q38_hybrid_startup_layout_test.cpp'
cmd = [str(cxx), '-std=gnu++17', '-O2', '-Wall', '-Wextra', '-Werror',
       '-DPIONEER_Q38_GLOBAL_CHAIN_BF16_FPU=1', '-DPIONEER_Q38_GLOBAL_CHAIN_PRECISE_ACTIVATION=1',
       '-DPIONEER_Q38_GLOBAL_CHAIN_HYBRID_BFP=1', '-DPIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE=1',
       '-DPIONEER_Q38_TOKEN_CANCEL_BOUNDARY=1', *includes,
       '-B' + str(release/'system/glue'), '-L' + str(release/'system/libroot'),
       '-L' + str(release/'kits'), *libraries, str(source), '-o', str(output)]
subprocess.run(cmd, check=True)
print('QGW5_STARTUP_TEST_SHA256', hashlib.sha256(output.read_bytes()).hexdigest())
print('BUILD ONLY; NO DEVICE ACCESS OR EXECUTION')
