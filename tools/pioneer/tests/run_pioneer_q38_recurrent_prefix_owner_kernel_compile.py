#!/usr/bin/env python3
"""RISC-V kernel template instantiation, NEVER a native context/driver."""
import hashlib
import os
import pathlib
import subprocess
import sys
assert len(sys.argv)==3
sdk,out=map(pathlib.Path,sys.argv[1:]);here=pathlib.Path(__file__).resolve().parent
assert sdk.is_absolute() and out.is_absolute() and not out.exists() and not out.is_symlink();out.mkdir(mode=0o700)
headers=sdk/'objects/haiku/riscv64/packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop/headers'
tool=str(sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku');obj=out/'prefix-owner.o'
subprocess.run([tool+'-g++','-std=gnu++17','-O2','-Wall','-Wextra','-Werror','-fno-exceptions','-fno-rtti','-ffreestanding',
 '-fstack-usage','-mcmodel=medany','-fpic','-D_KERNEL_MODE','-I'+str(headers),'-idirafter',str(headers/'posix'),
 '-ffile-prefix-map='+str(here.parents[2])+'/=/pioneer-source/','-c',str(here/'pioneer_q38_recurrent_prefix_owner_kernel_compile.cpp'),'-o',str(obj)],check=True)
rows=[line.split('\t') for line in obj.with_suffix('.su').read_text().splitlines()]
assert rows and all(len(r)==3 and r[2]=='static' and int(r[1])<=2048 for r in rows),rows
defined=subprocess.check_output([tool+'-nm','--defined-only',str(obj)],text=True)
for name in ('pioneer_q38_prefix_owner_template_prepare','pioneer_q38_prefix_owner_template_run','pioneer_q38_prefix_owner_template_close'):assert name in defined
for name in ('api_version','init_hardware','init_driver','uninit_driver','publish_devices','find_device'):
 assert not any(line.split()[-1]==name for line in defined.splitlines() if line.split())
undefined=subprocess.check_output([tool+'-nm','-u',str(obj)],text=True)
assert 'PrefixOwnerKernelCompileIO' in undefined
assert 'PioneerRecurrentPrefixOriginalReference' not in defined
print(hashlib.sha256(obj.read_bytes()).hexdigest()+' prefix-owner.o')
print('PASS real RISC-V SDK prefix execution owner TEMPLATE/prepare+ONEstart+autonomousSTOP+close/maxStaticFrame'+str(max(int(r[1]) for r in rows))+';undefined PREFIX IO hooks,NO native context,driver ABI,launch,hardware or TPS proof')
