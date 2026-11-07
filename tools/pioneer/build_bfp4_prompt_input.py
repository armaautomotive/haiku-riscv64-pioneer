#!/usr/bin/env python3
"""Build CPU-only first-prompt-input observer; never opens or installs a driver."""
import argparse
import hashlib
import os
from pathlib import Path
import subprocess

assert __debug__ and not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('haiku_build',type=Path);p.add_argument('output',type=Path);a=p.parse_args()
assert a.haiku_build.is_absolute() and a.haiku_build.is_dir()
assert a.output.is_absolute() and not a.output.exists() and not a.output.is_symlink()
a.output.mkdir(mode=0o700)
here=Path(__file__).resolve().parent;root=here.parent.parent;build=a.haiku_build
sdk=build/'objects/haiku/riscv64/packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop'
release=build/'objects/haiku/riscv64/release'
cxx=str(build/'cross-tools-riscv64/bin/riscv64-unknown-haiku-g++')
includes=['-I'+str(sdk/'headers'),'-idirafter',str(sdk/'headers/posix')]
for path in [sdk/'headers/os',*sorted((sdk/'headers/os').iterdir())]:
 if path.is_dir():includes+=['-isystem',str(path)]
libs=['-L'+str(path) for path in sorted((build/'build_packages').glob('gcc_syslibs-*/lib'))]
binary=a.output/'pioneer-bfp4-prompt-input'
cmd=[cxx,'-std=c++17','-O2','-ffp-contract=off','-Wall','-Wextra','-Werror','-Wno-misleading-indentation','-DPIONEER_Q38_BFP4_QUALITY_DIAGNOSTIC=1',
 *includes,'-I'+str(root/'headers/private/shared'),'-I'+str(root/'headers/private/libroot'),
 '-B'+str(release/'system/glue'),'-L'+str(release/'system/libroot'),'-L'+str(release/'kits'),*libs,
 str(here/'tests/pioneer_q38_bfp4_prompt_input.cpp'),str(here/'tests/pioneer_q38_cpu_model_reference.cpp'),
 str(here/'PioneerModelWeightCache.cpp'),str(release/'kits/shared/libshared.a'),'-lbe','-o',str(binary)]
subprocess.run(cmd,check=True,cwd=a.output)
quality=a.output/'pioneer-bfp4-prompt-quality'
quality_cmd=[cxx,'-std=c++17','-O2','-ffp-contract=off','-Wall','-Wextra','-Werror',*includes,
 '-I'+str(root/'headers/private/shared'),'-I'+str(root/'headers/private/libroot'),
 '-B'+str(release/'system/glue'),'-L'+str(release/'system/libroot'),'-L'+str(release/'kits'),*libs,
 str(here/'pioneer_bfp4_prompt_quality.cpp'),str(here/'PioneerModelWeightCache.cpp'),
 str(release/'kits/shared/libshared.a'),'-lbe','-o',str(quality)]
subprocess.run(quality_cmd,check=True,cwd=a.output)
(a.output/'build-argv.txt').write_text('\t'.join(cmd)+'\n'+'\t'.join(quality_cmd)+'\n')
print(hashlib.sha256(binary.read_bytes()).hexdigest(),binary.name)
print(hashlib.sha256(quality.read_bytes()).hexdigest(),quality.name)
print('BUILD ONLY CPU prompt-input observer; NO device, model quality or speed proof')
