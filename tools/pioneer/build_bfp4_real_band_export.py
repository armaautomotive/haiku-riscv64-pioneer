#!/usr/bin/env python3
"""Build CPU-only private BFP4 sample exporter; no model or device opened."""
import argparse
import hashlib
import os
from pathlib import Path
import subprocess

assert __debug__ and not os.environ.get('PYTHONOPTIMIZE')
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('haiku_build',type=Path)
parser.add_argument('output',type=Path)
args=parser.parse_args()
assert args.haiku_build.is_absolute() and args.haiku_build.is_dir()
assert args.output.is_absolute() and not args.output.exists() and not args.output.is_symlink()
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
here=Path(__file__).resolve().parent
root=here.parent.parent
sdk=args.haiku_build/'objects/haiku/riscv64/packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop'
release=args.haiku_build/'objects/haiku/riscv64/release'
cxx=str(args.haiku_build/'cross-tools-riscv64/bin/riscv64-unknown-haiku-g++')
includes=['-I'+str(sdk/'headers'),'-idirafter',str(sdk/'headers/posix')]
for path in [sdk/'headers/os',*sorted((sdk/'headers/os').iterdir())]:
    if path.is_dir():
        includes+=['-isystem',str(path)]
libs=['-L'+str(path) for path in sorted((args.haiku_build/'build_packages').glob('gcc_syslibs-*/lib'))]
args.output.mkdir(mode=0o700)
common=['-std=c++17','-O2','-ffp-contract=off','-Wall','-Wextra','-Werror']
sources=[str(here/'pioneer_bfp4_real_band_export.cpp'),str(here/'PioneerModelWeightCache.cpp')]
native=[os.environ.get('CXX','c++'),*common,*sources,'-o',str(args.output/'export-mac')]
haiku=[cxx,*common,*includes,'-I'+str(root/'headers/private/shared'),'-I'+str(root/'headers/private/libroot'),
       '-B'+str(release/'system/glue'),'-L'+str(release/'system/libroot'),'-L'+str(release/'kits'),*libs,
       *sources,str(release/'kits/shared/libshared.a'),'-lbe','-o',str(args.output/'export-haiku')]
for command in (native,haiku):
    subprocess.run(command,check=True)
for macro in ('_KERNEL_MODE','PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE','PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_FULL_MODEL_CPU_REFERENCE'):
    for value in ('0','1'):
        result=subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-fsyntax-only','-D'+macro+'='+value,sources[0]],capture_output=True,text=True)
        assert result.returncode and 'BFP4 real-band export is a separate CPU diagnostic' in result.stderr,(macro,value,result.stderr)
result=subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-ffast-math','-fsyntax-only',sources[0]],capture_output=True,text=True)
assert result.returncode and 'BFP4 real-band export is a separate CPU diagnostic' in result.stderr
empty=args.output/'private-test'
empty.mkdir(mode=0o700)
result=subprocess.run([str(args.output/'export-mac'),str(args.output/'nonexistent.gguf'),str(empty)],capture_output=True,text=True)
assert result.returncode==1 and 'cannot open original model' in result.stderr and not list(empty.iterdir())
(args.output/'build-argv.txt').write_text('\n'.join('\t'.join(c) for c in (native,haiku))+'\n')
for name in ('export-mac','export-haiku'):
    print(hashlib.sha256((args.output/name).read_bytes()).hexdigest(),name)
print('PASS BUILD_ONLY private CPU exporter, 11 role refusals and missing-source NO OUTPUT; NO model read or GPU access')
