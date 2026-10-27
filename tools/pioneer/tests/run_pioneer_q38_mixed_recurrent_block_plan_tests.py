#!/usr/bin/env python3
"""Canonical whole recurrent DATA plan; host ASan/UBSan+real kernel compile."""
import os
import pathlib
import subprocess
import sys
import tempfile
args=sys.argv[1:];assert len(args) in (0,2) and (not args or args[0]=='--sdk')
sdk=pathlib.Path(args[1]) if args else None
here=pathlib.Path(__file__).resolve().parent
with tempfile.TemporaryDirectory(prefix='pioneer-recurrent-block-plan-',dir='/private/tmp') as tmp:
 binary=pathlib.Path(tmp)/'test'
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-ffp-contract=off',
  '-fsanitize=address,undefined',str(here/'pioneer_q38_mixed_recurrent_block_plan_test.cpp'),'-o',str(binary)],check=True)
 subprocess.run([str(binary)],check=True,env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
 if sdk:
  headers=sdk/'objects/haiku/riscv64/packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop/headers';obj=pathlib.Path(tmp)/'kernel.o'
  subprocess.run([str(sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku-g++'),'-std=gnu++17','-O2','-Wall','-Wextra','-Werror',
   '-fno-exceptions','-fno-rtti','-ffreestanding','-fstack-usage','-mcmodel=medany','-fpic','-D_KERNEL_MODE','-DPIONEER_Q38_GDN_MUTABLE_STATE=1',
   '-I'+str(headers),'-idirafter',str(headers/'posix'),'-c',str(here/'pioneer_q38_mixed_recurrent_block_plan_kernel_compile.cpp'),'-o',str(obj)],check=True)
  rows=[line.split('\t') for line in obj.with_suffix('.su').read_text().splitlines()]
  assert rows and all(len(r)==3 and r[2]=='static' and int(r[1])<=2048 for r in rows),rows
  print('PASS actual RISC-V kernel QRB1 DATA-plan instantiation/heap-only/maxStaticFrame'+str(max(int(r[1]) for r in rows))+';NO driver linkage/IO/native authority')
