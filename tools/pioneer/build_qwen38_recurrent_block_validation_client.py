#!/usr/bin/env python3
"""Build/inspect separate CPU-reference DEBUG72 client;NEVER run/install."""
import hashlib
import os
import pathlib
import subprocess
import sys
if len(sys.argv)!=3 or any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ):raise SystemExit(2)
build,out=map(pathlib.Path,sys.argv[1:]);here=pathlib.Path(__file__).resolve().parent;project=here.parents[1]
assert build.is_absolute() and out.is_absolute() and not out.exists() and not out.is_symlink();out.mkdir(mode=0o700)
sdk=build/'objects/haiku/riscv64/packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop'
release=build/'objects/haiku/riscv64/release';tool=build/'cross-tools-riscv64/bin/riscv64-unknown-haiku-';cxx=str(tool)+'g++'
includes=['-I'+str(sdk/'headers'),'-idirafter',str(sdk/'headers/posix')]
for path in [sdk/'headers/os',*sorted((sdk/'headers/os').iterdir())]:
 if path.is_dir():includes+=['-isystem',str(path)]
libraries=['-L'+str(path) for path in sorted((build/'build_packages').glob('gcc_syslibs-*/lib'))]
flags=['-std=c++17','-O2','-ffp-contract=off','-Wall','-Wextra','-Werror','-fno-exceptions','-fno-rtti','-fstack-usage',
 '-ffile-prefix-map='+str(project)+'/=/pioneer-source/']
objects=[]
for name,path,selection in [('original',here/'pioneer_q38_recurrent_block_original_source.cpp',[]),
 ('client',here/'pioneer_q38_recurrent_block_validation.cpp',['-DPIONEER_Q38_RECURRENT_BLOCK_VALIDATION_CLIENT=1','-DPIONEER_Q38_CPU_VALIDATION=1'])]:
 obj=out/(name+'.o');subprocess.run([cxx,*flags,*selection,*includes,'-c',str(path),'-o',str(obj)],check=True);objects.append(obj)
binary=out/'pioneer-q38-recurrent-DEBUG72-VALIDATION-ONLY'
subprocess.run([cxx,*includes,*libraries,'-B'+str(release/'system/glue')+'/',
 '-L'+str(release/'system/libroot'),*map(str,objects),'-o',str(binary)],check=True)
symbols=subprocess.check_output([str(tool)+'nm','-C',str(binary)],text=True)
client_symbols=subprocess.check_output([str(tool)+'nm','-C',str(objects[1])],text=True)
for forbidden in ('PioneerRecurrentReferenceProducts','PioneerQ38CanonicalModelPlan','PioneerQ38GdnBanks','PioneerSessionIO','PioneerStoreContext'):
 assert forbidden not in client_symbols,forbidden
for forbidden in ('PioneerStoreContext','PioneerSessionIO','PioneerQ38RecurrentBlockOwner','Q38WorkControl',
 'RecurrentBlockNativeTransport','ExecuteRecurrentOriginalNativeScenario','PioneerQ38MixedLiveValidationSession'):
 assert forbidden not in symbols,forbidden
assert 'PioneerRecurrentOriginalSource' in symbols and 'PioneerQ38RecurrentBlockValidationSession' in symbols and 'ioctl' in symbols
sizes=[int(line.split('\t')[1]) for path in out.glob('*.su') for line in path.read_text().splitlines() if len(line.split('\t'))==3]
assert sizes and max(sizes)<=32768 # CPU reference USER stack,never a kernel frame
for path in [*objects,binary]:print(hashlib.sha256(path.read_bytes()).hexdigest()+' '+path.name)
print('PASS separate Haiku riscv64 DEBUG72 CPU-reference/source+actual-ioctl client compile/link/max USER frame'+str(max(sizes)))
print('PASS client object has NO inline private CPU products,native context,canonical mutable-bank geometry or GDN-owner implementation')
print('BUILD/INSPECT ONLY;NO installation,execution,device access,physical/full-model/Work/TPS qualification')
