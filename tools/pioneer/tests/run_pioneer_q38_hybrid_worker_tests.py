#!/usr/bin/env python3
"""Rebuild and exercise the experimental QGW5 dispatcher off-device."""
import hashlib
import os
from pathlib import Path
import subprocess
import sys
import tempfile

args=sys.argv[1:];reuse='--reuse-input' in args
if reuse:assert args.count('--reuse-input')==1;args.remove('--reuse-input')
assert __debug__ and len(args)==1 and not os.environ.get('PYTHONOPTIMIZE')
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
sdk=Path(args[0]);assert sdk.is_absolute() and sdk.is_dir()
here=Path(__file__).resolve().parent;root=here.parent
compiler=os.environ.get('CXX','c++')
def run(*args,quiet=False):
 subprocess.run([str(x) for x in args],check=True,stdout=subprocess.DEVNULL if quiet else None)
with tempfile.TemporaryDirectory(prefix='pioneer-qgw5-suite-',dir='/private/tmp') as temporary:
 temp=Path(temporary)
 rootNew=rootOld=None
 if reuse:
  sys.path.insert(0,str(root));from PioneerQ38HybridProgramPins import ROOT_PINS
  rootOld=temp/'root-original';rootNew=temp/'root-reuse'
  rootFlags=['--inspect-candidate','--cancel-boundary','--f32-hidden','--unroll-dense-seals','--boundary-seals',
   '--unroll-quiet','--worker-local-source-guards','--precise-activation','--single-closed-audit',
   '--worker-sealed-admission','--bf16-fpu','--hybrid-bfp']
  for extra in (['--hybrid-reuse-input'],[*rootFlags,'--hybrid-reuse-input','--fast-rms']):
   refused=temp/'refused-root'
   result=subprocess.run([sys.executable,str(root/'build_qwen38_token_root.py'),str(sdk),str(refused),*extra],capture_output=True)
   assert result.returncode and not refused.exists()
  run(sys.executable,root/'build_qwen38_token_root.py',sdk,rootOld,*rootFlags,quiet=True)
  run(sys.executable,root/'build_qwen38_token_root.py',sdk,rootNew,*rootFlags,'--hybrid-reuse-input',quiet=True)
  newPins=(('boot.bin',4,'32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca'),
   ('text.bin',130944,'8ccaff4af155fd85ab6edf3f6aa7af59645b3d583e688e8453e2951d71316c6d'),
   ('terminal.bin',8,'6eb1d84addfac7202ad4f0d747c6e64d3b92c03025651c7dabe6c56c9cba95d5'),
   ('conv.bin',21328,'3b4bdb7e7f7909b5f5a0e115adbf94d3871bc8928bb0d004e78263687eb3c824'),
   ('bf16.bin',33588,'3a5a833ed31ae2fd452a1a52e3e79f00fbe28d1f571bc90bd0c361976c18397d'),
   ('program.elf',217036,'3ccf7b407725e9e408d9370b0a1795ca4abaaa69edaf5605ef98b40a5082cc84'))
  nm=str(sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku-nm')
  for folder,pins in ((rootOld,ROOT_PINS),(rootNew,newPins)):
   for name,size,digest in pins:
    data=(folder/name).read_bytes();assert len(data)==size and hashlib.sha256(data).hexdigest()==digest,(folder,name)
   assert (folder/'program.symbols').read_bytes()==subprocess.check_output([nm,'-n',str(folder/'program.elf')])
  (rootNew/'program.demangled').write_bytes(subprocess.check_output([nm,'-nC',str(rootNew/'program.elf')]))
  rootJobs=temp/'root-jobs'
  run(compiler,'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-ffp-contract=off','-fsanitize=address,undefined',
   '-DPIONEER_Q38_HYBRID_INPUT_REUSE_INSPECTION=1',here/'pioneer_q38_hybrid_actual_root_test.cpp','-o',rootJobs)
  run(rootJobs,rootNew)
 for bits in (4,8):
  image=temp/('worker'+str(bits));leaf=temp/('leaf'+str(bits));binary=temp/('test'+str(bits))
  run(sys.executable,root/'build_q38_hybrid_worker.py',sdk,image,*(['--test-bfp8-layer0'] if bits==8 else []),*(['--reuse-input'] if reuse else []),quiet=True)
  run(sys.executable,root/('build_bfp'+str(bits)+'_worker_leaf.py'),sdk,leaf,*(['--reuse-input'] if reuse and bits==4 else []),quiet=True)
  flags=['-DPIONEER_TEST_BFP8=1','-DPIONEER_Q38_HYBRID_TEST_BFP8_LAYER0=1'] if bits==8 else []
  if reuse:flags+=['-DPIONEER_Q38_HYBRID_INPUT_REUSE_INSPECTION=1']
  if reuse and bits==4:flags+=['-DPIONEER_Q38_BFP4_INPUT_REUSE_EXPERIMENT=1']
  if reuse and bits==4:flags+=['-DPIONEER_Q38_TEST_LINKED_RECEIPT=1']
  run(compiler,'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-ffp-contract=off',
   '-fsanitize=address,undefined',*flags,here/'pioneer_q38_hybrid_worker_image_test.cpp','-o',binary)
  run(binary,image,leaf,*([rootNew,rootOld] if reuse and bits==4 else []))
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
  *(['-DPIONEER_Q38_HYBRID_INPUT_REUSE_INSPECTION=1'] if reuse else []),
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
 if reuse:
  profile=root/'PioneerQ38HybridInputReuseProfile.h'
  for macro in ('_KERNEL_MODE','PIONEER_DRIVER_BUILD_TAG','PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE',
                'PIONEER_Q38_FULL_OFFLOAD_NATIVE','PIONEER_Q38_FULL_OFFLOAD_CLIENT','PIONEER_Q38_TOKEN_NATIVE_CONTEXT',
                'PIONEER_Q38_BF16_FULL_NATIVE','PIONEER_Q38_BF16_FULL_CLIENT','PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_FULL_MODEL_CPU_REFERENCE'):
   result=subprocess.run([compiler,'-std=c++17','-DPIONEER_Q38_HYBRID_INPUT_REUSE_INSPECTION=1','-D'+macro,'-E','-x','c++',str(profile)],
    stdout=subprocess.DEVNULL,stderr=subprocess.PIPE,text=True)
   assert result.returncode and 'Hybrid input-reuse accounting is for the isolated inspection image only' in result.stderr,macro
  result=subprocess.run([compiler,'-std=c++17','-DPIONEER_Q38_HYBRID_INPUT_REUSE_INSPECTION=0','-E','-x','c++',str(profile)],
   stdout=subprocess.DEVNULL,stderr=subprocess.PIPE,text=True)
  assert result.returncode and 'Hybrid input-reuse accounting is for the isolated inspection image only' in result.stderr
  result=subprocess.run([compiler,'-std=c++17','-DPIONEER_Q38_HYBRID_INPUT_REUSE_INSPECTION=1',
   '-DPIONEER_Q38_HYBRID_WORKER_IMAGE=1','-E',str(root/'blackhole_q38_bfp4_worker_leaf.cpp')],
   stdout=subprocess.DEVNULL,stderr=subprocess.PIPE,text=True)
  assert result.returncode and 'Hybrid input-reuse inspection requires the matching BFP4 leaf' in result.stderr
  result=subprocess.run([compiler,'-std=c++17','-DPIONEER_Q38_HYBRID_INPUT_REUSE_INSPECTION=1',
   '-E',str(root/'blackhole_qwen38_global_worker.cpp')],stdout=subprocess.DEVNULL,stderr=subprocess.PIPE,text=True)
  assert result.returncode and 'Hybrid input-reuse accounting requires its distinct worker inspection image' in result.stderr
print('PASS QGW5'+(' input-reuse inspection' if reuse else '')+': real linked RV32 dispatcher under simulation; BFP8 uses test-only layer0 mask; NO CARD OR FULL MODEL')
