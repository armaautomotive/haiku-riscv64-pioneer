#!/usr/bin/env python3
"""Distinct QPX1 held journal. Simulated endpoints, NEVER native/device IO."""
import os
import pathlib
import re
import subprocess
import sys
import tempfile
args=sys.argv[1:];sdk=None
if len(args)>=2 and args[-2]=='--sdk':sdk=pathlib.Path(args[-1]);args=args[:-2]
assert args in ([],['--positive'],['--faults'])
here=pathlib.Path(__file__).resolve().parent
flags=re.findall(r'-D[A-Z][A-Z0-9_]*(?:=[0-9]+)?',(here/'run_pioneer_q38_ffn_persistent_image_tests.sh').read_text())
with tempfile.TemporaryDirectory(prefix='pioneer-prefix-memory-',dir='/private/tmp') as tmp:
 binary=pathlib.Path(tmp)/'test'
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror',
  '-fsanitize=address,undefined',*flags,str(here/'pioneer_q38_recurrent_prefix_memory_test.cpp'),'-o',str(binary)],check=True)
 for extra in ([[]] if args==['--positive'] else [['--faults']] if args==['--faults'] else [[],['--faults']]):
  subprocess.run([str(binary),*extra],check=True,env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
 if sdk:
  headers=sdk/'objects/haiku/riscv64/packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop/headers'
  obj=pathlib.Path(tmp)/'kernel.o';tool=str(sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku')
  subprocess.run([tool+'-g++','-std=gnu++17','-O2','-Wall','-Wextra','-Werror','-fno-exceptions','-fno-rtti',
   '-ffreestanding','-fstack-usage','-mcmodel=medany','-fpic','-D_KERNEL_MODE','-I'+str(headers),'-idirafter',str(headers/'posix'),
   '-c',str(here/'pioneer_q38_recurrent_prefix_memory_kernel_compile.cpp'),'-o',str(obj)],check=True)
  rows=[line.split('\t') for line in obj.with_suffix('.su').read_text().splitlines()]
  assert rows and all(len(r)==3 and r[2]=='static' and int(r[1])<=2048 for r in rows),rows
  defined=subprocess.check_output([tool+'-nm','--defined-only',str(obj)],text=True)
  assert all(name in defined for name in ('pioneer_q38_recurrent_prefix_memory_template_prepare','pioneer_q38_recurrent_prefix_memory_template_close'))
  assert not any(line.split()[-1] in ('api_version','init_hardware','init_driver','uninit_driver','publish_devices','find_device') for line in defined.splitlines() if line.split())
  unresolved=subprocess.check_output([tool+'-nm','-u',str(obj)],text=True)
  assert 'PrefixMemoryKernelCompileIO' in unresolved
  assert 'RecurrentMemoryKernelCompileIO' not in unresolved
  print('PASS real RISC-V SDK QPX1 journal TEMPLATE/maxStaticFrame'+str(max(int(r[1]) for r in rows))+
   '/separate undefined PREFIX IO hooks/no conventional ABI;NOT genuine native context or loadable driver')
