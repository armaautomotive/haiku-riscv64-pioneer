#!/usr/bin/env python3
"""Cross-build CPU-only Qwen reference for Haiku; never execute or access a card."""
import hashlib
import os
from pathlib import Path
import subprocess
import sys

if len(sys.argv)!=3 or any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ):
    raise SystemExit('usage: ABS_HAIKU_BUILD NEW_ABS_OUTPUT')
build,output=map(Path,sys.argv[1:])
if not build.is_absolute() or not build.is_dir() or not output.is_absolute() or output.exists() or output.is_symlink():
    raise SystemExit('invalid build or output')
here=Path(__file__).resolve().parent;root=here.parent.parent.parent
sdk=build/'objects/haiku/riscv64/packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop'
release=build/'objects/haiku/riscv64/release'
tool=build/'cross-tools-riscv64/bin/riscv64-unknown-haiku-g++'
libs=sorted((build/'build_packages').glob('gcc_syslibs-*/lib'))
if not tool.is_file() or not (sdk/'headers/posix').is_dir() or not libs:
    raise SystemExit('incomplete Haiku SDK')
includes=['-I'+str(sdk/'headers'),'-idirafter',str(sdk/'headers/posix')]
for path in [sdk/'headers/os',*sorted((sdk/'headers/os').iterdir())]:
    if path.is_dir():includes+=['-isystem',str(path)]
sources=[here/'pioneer_q38_cpu_model_original.cpp',here/'pioneer_q38_cpu_model_reference.cpp',
         here/'pioneer_q38_whole_original_bytes.cpp']
watched=sources+[here/'pioneer_q38_cpu_reference_prompt.h']
before={p:hashlib.sha256(p.read_bytes()).digest() for p in watched}
command=[str(tool),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-Wno-misleading-indentation',
         '-ffp-contract=off',*includes,'-I'+str(root/'headers/private/shared'),'-I'+str(root/'headers/private/libroot'),
         '-B'+str(release/'system/glue'),'-L'+str(release/'system/libroot'),'-L'+str(release/'kits'),
         *['-L'+str(p) for p in libs],*map(str,sources),'-o',str(output)]
subprocess.run(command,check=True)
assert all(hashlib.sha256(p.read_bytes()).digest()==before[p] for p in watched)
print('CPU_REFERENCE_BUILD_ONLY',hashlib.sha256(output.read_bytes()).hexdigest(),
      '; NO EXECUTION, MODEL VALIDATION, DEVICE OR SPEED CLAIM')
