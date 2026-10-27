#!/usr/bin/env python3
"""All recurrent canonical recipes vs independent GGUF loader; no device IO."""
import os
import pathlib
import subprocess
import sys
import tempfile
args=sys.argv[1:];sdk=None
if args[:1]==['--sdk']:
 assert len(args) in (2,3)
 sdk=pathlib.Path(args[1]);args=args[2:]
assert len(args)<=1
here=pathlib.Path(__file__).resolve().parent
with tempfile.TemporaryDirectory(prefix='pioneer-recurrent-projection-plan-',dir='/private/tmp') as temp:
 binary=pathlib.Path(temp)/'test'
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-ffp-contract=off','-Wall','-Wextra','-Werror',
  '-fsanitize=address,undefined',str(here/'pioneer_q38_mixed_recurrent_projection_plan_test.cpp'),'-o',str(binary)],check=True)
 subprocess.run([str(binary),*args],check=True,
  env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
 if sdk:
  headers=sdk/'objects/haiku/riscv64/packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop/headers'
  obj=pathlib.Path(temp)/'kernel.o'
  subprocess.run([str(sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku-g++'),'-std=gnu++17','-O2','-Wall','-Wextra','-Werror',
   '-fno-exceptions','-fno-rtti','-ffreestanding','-fstack-usage','-mcmodel=medany','-fpic','-D_KERNEL_MODE','-DPIONEER_Q38_GDN_MUTABLE_STATE=1',
   '-I'+str(headers),'-idirafter',str(headers/'posix'),'-c',str(here/'pioneer_q38_mixed_recurrent_projection_plan_kernel_compile.cpp'),'-o',str(obj)],check=True)
  rows=[line.split('\t') for line in obj.with_suffix('.su').read_text().splitlines()]
  assert rows and all(len(r)==3 and r[2]=='static' and int(r[1])<=2048 for r in rows),rows
  print('PASS actual RISC-V kernel DATA-plan instantiation/heap-only jobs/maxStaticFrame'+str(max(int(r[1]) for r in rows))+';NO driver linkage/IO/native authority')
