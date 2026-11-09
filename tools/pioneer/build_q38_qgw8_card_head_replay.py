#!/usr/bin/env python3
"""Cross-build the isolated, CPU-only card-hidden head replay for Haiku."""
import hashlib
import os
from pathlib import Path
import subprocess
import sys

if not __debug__ or len(sys.argv) != 3 or any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ):
    raise SystemExit('usage: ABS_HAIKU_BUILD NEW_ABS_OUTPUT')
build, output = map(Path, sys.argv[1:])
if not build.is_absolute() or not build.is_dir() or not output.is_absolute() or output.exists() or output.is_symlink():
    raise SystemExit('invalid build or new output path')
here = Path(__file__).resolve().parent; root = here.parent.parent
sdk = build/'objects/haiku/riscv64/packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop'
release = build/'objects/haiku/riscv64/release'
cxx = build/'cross-tools-riscv64/bin/riscv64-unknown-haiku-g++'
if not cxx.is_file() or not sdk.is_dir() or not (release/'kits/shared/libshared.a').is_file():
    raise SystemExit('Haiku toolchain or SDK missing')
output.mkdir(mode=0o700)
includes = ['-I'+str(sdk/'headers'), '-idirafter', str(sdk/'headers/posix')]
for path in [sdk/'headers/os', *sorted((sdk/'headers/os').iterdir())]:
    if path.is_dir(): includes += ['-isystem', str(path)]
includes += ['-I'+str(root/'headers/private/shared'), '-I'+str(root/'headers/private/libroot')]
libs = ['-L'+str(path) for path in sorted((build/'build_packages').glob('gcc_syslibs-*/lib'))]
sources = [here/'tests/pioneer_q38_qgw8_card_head_replay.cpp', here/'tests/pioneer_q38_head_bfp8_cpu_backend.cpp',
           here/'tests/pioneer_q38_whole_original_bytes.cpp', here/'PioneerModelWeightCache.cpp']
pins = {p: hashlib.sha256(p.read_bytes()).hexdigest() for p in sources}
binary = output/'pioneer-qgw8-card-head-replay'
cmd = [str(cxx), '-std=c++17', '-O2', '-ffp-contract=off', '-Wall', '-Wextra', '-Werror',
       '-Wno-misleading-indentation', '-DPIONEER_Q38_CARD_HEAD_REPLAY=1', *includes,
       '-B'+str(release/'system/glue'), '-L'+str(release/'system/libroot'), '-L'+str(release/'kits'),
       *libs, *(str(p) for p in sources), str(release/'kits/shared/libshared.a'), '-lbe', '-o', str(binary)]
subprocess.run(cmd, check=True, cwd=output)
if any(hashlib.sha256(p.read_bytes()).hexdigest() != digest for p, digest in pins.items()):
    raise RuntimeError('source changed while building')
(output/'build-argv.txt').write_text('\n'.join(cmd)+'\n')
print('HAIKU_CPU_HEAD_REPLAY_SHA256', hashlib.sha256(binary.read_bytes()).hexdigest())
print('BUILD ONLY: CPU diagnostic; no model, device or inference accessed')
