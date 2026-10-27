#!/usr/bin/env python3
"""Compile/link CPU-ONLY original-source unit for Haiku;NEVER run/install it."""
import hashlib
import os
import pathlib
import subprocess
import sys
if len(sys.argv)!=3 or any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ):raise SystemExit(2)
build,out=map(pathlib.Path,sys.argv[1:]);here=pathlib.Path(__file__).resolve().parent;project=here.parents[2]
assert build.is_absolute() and out.is_absolute() and not out.exists() and not out.is_symlink();out.mkdir(mode=0o700)
sdk=build/'objects/haiku/riscv64/packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop'
release=build/'objects/haiku/riscv64/release';tool=build/'cross-tools-riscv64/bin/riscv64-unknown-haiku-'
cxx=str(tool)+'g++'
includes=['-I'+str(sdk/'headers'),'-idirafter',str(sdk/'headers/posix')]
for path in [sdk/'headers/os',*sorted((sdk/'headers/os').iterdir())]:
 if path.is_dir():includes+=['-isystem',str(path)]
libraries=['-L'+str(path) for path in sorted((build/'build_packages').glob('gcc_syslibs-*/lib'))]
flags=['-std=c++17','-O2','-ffp-contract=off','-Wall','-Wextra','-Werror','-fno-exceptions','-fno-rtti','-fstack-usage',
 '-ffile-prefix-map='+str(project)+'/=/pioneer-source/']
objects=[]
for name,path in [('original',here.parent/'pioneer_q38_recurrent_block_original_source.cpp'),('source-unit',here/'pioneer_q38_recurrent_block_original_source_test.cpp')]:
 obj=out/(name+'.o');subprocess.run([cxx,*flags,*includes,'-c',str(path),'-o',str(obj)],check=True);objects.append(obj)
binary=out/'recurrent-original-CPU-SOURCE-UNIT-NEVER-INSTALLED'
subprocess.run([cxx,*includes,*libraries,'-B'+str(release/'system/glue')+'/',
 '-L'+str(release/'system/libroot'),*map(str,objects),'-o',str(binary)],check=True)
symbols=subprocess.check_output([str(tool)+'nm','-C',str(binary)],text=True)
for forbidden in ('PioneerStoreContext','PioneerSessionIO','PioneerQ38RecurrentBlockOwner','Q38WorkControl','ioctl'):
 assert forbidden not in symbols,forbidden
assert 'PioneerRecurrentOriginalSource' in symbols and 'OpenPioneerQ38RecurrentBlockOriginalSource' in symbols
sizes=[int(line.split('\t')[1]) for path in out.glob('*.su') for line in path.read_text().splitlines() if len(line.split('\t'))==3]
assert sizes and max(sizes)<=32768 # user-space CPU reference,NEVER a kernel stack
for path in [*objects,binary]:print(hashlib.sha256(path.read_bytes()).hexdigest()+' '+path.name)
print('PASS Haiku riscv64 CPU-only original source/reference+source-unit compile/link/maxFrame'+str(max(sizes))+';NO native owner,driver IO,installation,execution or physical/numerical/TPS qualification')
