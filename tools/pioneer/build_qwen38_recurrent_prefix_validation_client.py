#!/usr/bin/env python3
"""Build/inspect RESERVED DEBUG73 client+separate CPU oracle;NEVER run/install."""
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
for name,path,selection in [('original',here/'pioneer_q38_recurrent_prefix_original_source.cpp',[]),
 ('adapter',here/'pioneer_q38_recurrent_prefix_validation_source.cpp',[]),
 ('client',here/'pioneer_q38_recurrent_prefix_validation.cpp',['-DPIONEER_Q38_RECURRENT_PREFIX_VALIDATION_CLIENT=1','-DPIONEER_Q38_CPU_VALIDATION=1'])]:
 obj=out/(name+'.o');subprocess.run([cxx,*flags,*selection,*includes,'-c',str(path),'-o',str(obj)],check=True);objects.append(obj)
binary=out/'pioneer-q38-prefix-DEBUG73-VALIDATION-ONLY'
subprocess.run([cxx,*includes,*libraries,'-B'+str(release/'system/glue')+'/',
 '-L'+str(release/'system/libroot'),*map(str,objects),'-o',str(binary)],check=True)
symbols=subprocess.check_output([str(tool)+'nm','-C',str(binary)],text=True)
client_symbols=subprocess.check_output([str(tool)+'nm','-C',str(objects[2])],text=True)
adapter_symbols=subprocess.check_output([str(tool)+'nm','-C',str(objects[1])],text=True)
for forbidden in ('PioneerRecurrentPrefixReferenceProducts','PioneerRecurrentPrefixOriginalSource','PioneerQ38CanonicalModelPlan',
 'PioneerQwen38PackedPlan','PioneerQ38GdnBanks','PioneerSessionIO','PioneerStoreContext'):
 assert forbidden not in client_symbols,forbidden
for forbidden in ('PioneerRecurrentPrefixReferenceProducts','PioneerQwen38PackedPlan','PioneerSessionIO','PioneerStoreContext','ioctl'):
 assert forbidden not in adapter_symbols,forbidden
for forbidden in ('PioneerStoreContext','PioneerSessionIO','PioneerQ38RecurrentPrefixOwner','Q38WorkControl',
 'RecurrentPrefixNativeTransport','ExecutePrefixOriginalNativeScenario','PioneerQ38RecurrentBlockValidationSession'):
 assert forbidden not in symbols,forbidden
assert 'PioneerRecurrentPrefixOriginalSource' in symbols and 'PioneerQ38RecurrentPrefixValidationSession' in symbols and 'ioctl' in symbols
sizes=[int(line.split('\t')[1]) for path in out.glob('*.su') for line in path.read_text().splitlines() if len(line.split('\t'))==3]
assert sizes and max(sizes)<=32768 # CPU-reference USER stack,never kernel frames
data=out/'protocol-data-kernel.o'
subprocess.run([cxx,*flags,*includes,'-D_KERNEL_MODE','-ffreestanding','-c',str(here/'tests/pioneer_q38_recurrent_prefix_validation_protocol_kernel_compile.cpp'),'-o',str(data)],check=True)
rows=[line.split('\t') for line in data.with_suffix('.su').read_text().splitlines()]
assert rows and all(len(r)==3 and r[2]=='static' and int(r[1])<=256 for r in rows),rows
data_symbols=subprocess.check_output([str(tool)+'nm','-C',str(data)],text=True)
assert 'pioneer_prefix_validation_request_data' in data_symbols
for forbidden in ('ioctl','PioneerSessionIO','PioneerStoreContext','init_driver','publish_devices'):assert forbidden not in data_symbols,forbidden
for path in [*objects,binary,data]:print(hashlib.sha256(path.read_bytes()).hexdigest()+' '+path.name)
print('PASS separate Haiku riscv64 RESERVED DEBUG73 CPU-source+adapter+actual-ioctl client compile/link/max USER frame'+str(max(sizes)))
print('PASS client object has NO private CPU products/source implementation/canonical mutable geometry or native owners;adapter has NO inline oracle or device IO')
print('PASS matching fixed20864B/96word kernel DATA ABI/max kernel frame'+str(max(int(r[1]) for r in rows))+';NO kernel driver/control implementation')
print('BUILD/INSPECT ONLY;physical role/selector/control/helper NOT implemented or granted here;NO installation,execution,device access,physical/full-model/Work/TPS qualification')
