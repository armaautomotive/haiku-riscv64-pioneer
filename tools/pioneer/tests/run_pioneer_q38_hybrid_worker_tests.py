#!/usr/bin/env python3
"""Rebuild and exercise the experimental QGW5 dispatcher off-device."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile

assert __debug__ and len(sys.argv)==2 and not os.environ.get('PYTHONOPTIMIZE')
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
sdk=Path(sys.argv[1]);assert sdk.is_absolute() and sdk.is_dir()
here=Path(__file__).resolve().parent;root=here.parent
compiler=os.environ.get('CXX','c++')
def run(*args,quiet=False):
 subprocess.run([str(x) for x in args],check=True,stdout=subprocess.DEVNULL if quiet else None)
with tempfile.TemporaryDirectory(prefix='pioneer-qgw5-suite-',dir='/private/tmp') as temporary:
 temp=Path(temporary)
 for bits in (4,8):
  image=temp/('worker'+str(bits));leaf=temp/('leaf'+str(bits));binary=temp/('test'+str(bits))
  run(sys.executable,root/'build_q38_hybrid_worker.py',sdk,image,*(['--test-bfp8-layer0'] if bits==8 else []),quiet=True)
  run(sys.executable,root/('build_bfp'+str(bits)+'_worker_leaf.py'),sdk,leaf,quiet=True)
  flags=['-DPIONEER_TEST_BFP8=1','-DPIONEER_Q38_HYBRID_TEST_BFP8_LAYER0=1'] if bits==8 else []
  run(compiler,'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-ffp-contract=off',
   '-fsanitize=address,undefined',*flags,here/'pioneer_q38_hybrid_worker_image_test.cpp','-o',binary)
  run(binary,image,leaf)
 binding=temp/'binding'
 run(compiler,'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',
  here/'pioneer_q38_hybrid_job_binding_test.cpp','-o',binding)
 run(binding)
 audit=temp/'old-snapshot-audit'
 run(compiler,'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',
  here/'pioneer_q38_hybrid_old_snapshot_audit.cpp','-o',audit)
 run(audit)
 tool=str(sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku-')
 probe=temp/'receipt.o'
 run(tool+'g++','-std=c++11','-O2','-Wall','-Wextra','-Werror','-ffreestanding','-fno-builtin',
  '-fno-exceptions','-fno-rtti','-fno-stack-protector','-fno-pic','-fno-pie',
  '-ffunction-sections','-fdata-sections','-fno-unwind-tables','-fno-asynchronous-unwind-tables',
  '-msmall-data-limit=0','-mno-relax','-march=rv32imf','-mabi=ilp32',
  '-c',here/'pioneer_q38_hybrid_receipt_probe.cpp','-o',probe)
 assert not subprocess.check_output([tool+'nm','-u',str(probe)]).strip(), 'receipt probe needs unsupported runtime helpers'
 rootobj=temp/'root-pairs.o';rootstart=temp/'root-start.o';rootelf=temp/'root-pairs.elf'
 run(tool+'g++','-std=c++11','-O2','-Wall','-Wextra','-Werror','-ffreestanding','-fno-builtin',
  '-fno-exceptions','-fno-rtti','-fno-stack-protector','-fno-pic','-fno-pie','-ffunction-sections','-fdata-sections',
  '-fno-unwind-tables','-fno-asynchronous-unwind-tables','-msmall-data-limit=0','-mno-relax','-march=rv32imf','-mabi=ilp32',
  '-c',here/'blackhole_q38_hybrid_root_pair_probe.cpp','-o',rootobj)
 run(tool+'as','-march=rv32imf','-mabi=ilp32','-mno-relax',root/'blackhole_q38_native_dram_band_start.S','-o',rootstart)
 run(tool+'ld','-m','elf32lriscv','--no-relax','--gc-sections','-T',root/'blackhole_q38_native_dram_band.ld',rootstart,rootobj,'-o',rootelf)
 assert not subprocess.check_output([tool+'nm','-u',str(rootelf)]).strip()
 image=temp/'root-pair-image';image.mkdir()
 for section in ('boot','text'):run(tool+'objcopy','-O','binary','-j','.'+section,rootelf,image/(section+'.bin'))
 (image/'symbols').write_bytes(subprocess.check_output([tool+'nm','-n',str(rootelf)]))
 binary=temp/'root-pair-test'
 run(compiler,'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-ffp-contract=off',
  '-fsanitize=address,undefined',here/'pioneer_q38_hybrid_root_pairs_image_test.cpp','-o',binary)
 run(binary,image)
 for macro in ('_KERNEL_MODE','PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE',
  'PIONEER_Q38_FULL_OFFLOAD_NATIVE','PIONEER_Q38_FULL_OFFLOAD_CLIENT','PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_FULL_MODEL_CPU_REFERENCE'):
  result=subprocess.run([compiler,'-std=c++17','-D'+macro,'-E',str(here/'pioneer_q38_hybrid_job_binding_test.cpp')],
   stdout=subprocess.DEVNULL,stderr=subprocess.PIPE,text=True)
  assert result.returncode and 'Experimental hybrid worker grammar' in result.stderr,macro
print('PASS QGW5: real linked RV32 dispatcher under simulation; BFP8 uses test-only layer0 mask; NO CARD OR FULL MODEL')
