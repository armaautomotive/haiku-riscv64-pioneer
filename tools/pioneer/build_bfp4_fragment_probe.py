#!/usr/bin/env python3
"""Build a CPU-only converted FFN fragment/layout probe; no device access."""
import argparse
import hashlib
import os
from pathlib import Path
import subprocess

assert __debug__ and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('haiku_build',type=Path);p.add_argument('output',type=Path);a=p.parse_args()
assert a.haiku_build.is_absolute() and a.haiku_build.is_dir()
assert a.output.is_absolute() and not a.output.exists() and not a.output.is_symlink()
a.output.mkdir(mode=0o700)
here=Path(__file__).resolve().parent;root=here.parent.parent;build=a.haiku_build
sdk=build/'objects/haiku/riscv64/packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop'
release=build/'objects/haiku/riscv64/release';cxx=str(build/'cross-tools-riscv64/bin/riscv64-unknown-haiku-g++')
includes=['-I'+str(sdk/'headers'),'-idirafter',str(sdk/'headers/posix')]
for path in [sdk/'headers/os',*sorted((sdk/'headers/os').iterdir())]:
 if path.is_dir():includes+=['-isystem',str(path)]
libs=['-L'+str(path) for path in sorted((build/'build_packages').glob('gcc_syslibs-*/lib'))]
binary=a.output/'pioneer-q38-bfp4-fragment-probe'
cmd=[cxx,'-std=c++17','-O2','-ffp-contract=off','-Wall','-Wextra','-Werror','-DPIONEER_Q38_BFP4_PREPARATION_DIAGNOSTIC=1',
 *includes,'-I'+str(root/'headers/private/shared'),'-I'+str(root/'headers/private/libroot'),
 '-B'+str(release/'system/glue'),'-L'+str(release/'system/libroot'),'-L'+str(release/'kits'),*libs,
 str(here/'pioneer_q38_bfp4_fragment_probe.cpp'),str(here/'PioneerModelWeightCache.cpp'),
 str(release/'kits/shared/libshared.a'),'-lbe','-o',str(binary)]
subprocess.run(cmd,check=True,cwd=a.output)
identity=hashlib.sha256()
for source in ('PioneerQ38Bfp4FragmentSource.h','PioneerBlackholeBlockFloat.h','PioneerQwen38Gguf.h','TenstorrentGgufQuant.h','TenstorrentQ4K.h'):
 data=(here/source).read_bytes();identity.update(source.encode()+b'\0');identity.update(len(data).to_bytes(8,'little'));identity.update(data)
for option in ('-dumpmachine','-dumpfullversion'):
 data=subprocess.check_output([cxx,option]);identity.update(option.encode()+b'\0');identity.update(len(data).to_bytes(8,'little'));identity.update(data)
identity.update(b'c++17/-O2/-ffp-contract=off/FE_TONEAREST')
fingerprint=identity.hexdigest();cacheProbe=a.output/'pioneer-q38-bfp4-cache-probe'
cacheCmd=[str(here/'pioneer_q38_bfp4_cache_probe.cpp') if item==str(here/'pioneer_q38_bfp4_fragment_probe.cpp')
          else str(cacheProbe) if item==str(binary) else item for item in cmd]
cacheCmd.insert(1,'-DPIONEER_BFP4_IMPLEMENTATION_SHA256="'+fingerprint+'"')
subprocess.run(cacheCmd,check=True,cwd=a.output)
(a.output/'build-argv.txt').write_text('\t'.join(cmd)+'\n'+'\t'.join(cacheCmd)+'\n')
print(hashlib.sha256(binary.read_bytes()).hexdigest(),binary.name)
print(hashlib.sha256(cacheProbe.read_bytes()).hexdigest(),cacheProbe.name)
print(fingerprint,'converter-implementation-fingerprint')
print('BUILD ONLY BFP4 fragment/layout diagnostic; NO DEVICE OR WORK')
