#!/usr/bin/env python3
"""Test hybrid job binding and actual RV32 BFP8 leaf, offboard only."""
import hashlib
import os
import re
from pathlib import Path
import stat
import subprocess
import sys
import tempfile

args=sys.argv[1:];watch='--watch-incoming' in args
if watch:
 assert args.count('--watch-incoming')==1;args.remove('--watch-incoming')
reuse='--reuse-input' in args
if reuse:
 assert args.count('--reuse-input')==1;args.remove('--reuse-input')
batch='--batch-input' in args
if batch:
 assert args.count('--batch-input')==1;args.remove('--batch-input')
headSamples='--qgw8-head-samples' in args
if headSamples:
 assert args.count('--qgw8-head-samples')==1;args.remove('--qgw8-head-samples')
assert not watch or reuse
assert not batch or not (reuse or watch)
assert __debug__ and len(args)==2
build,private=map(Path,args);assert build.is_absolute() and build.is_dir()
assert private.is_absolute() and not private.is_symlink() and stat.S_IMODE(private.stat().st_mode)==0o700
assert private.stat().st_uid==os.geteuid() and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
pins={'PioneerNativeBfpModelBandData.h':'cf8e2ca4d8489b1e2c9dbf3d23486a5b2b53a678f042629cbb4385e68daaa4f5',
      'PioneerNativeBfpModelBandMetadata.h':'a2c977bc4b7d517b5bc7f0ed188ed92eff16b3bc27dd0a59c5899e235c3d439c'}
if batch or headSamples:
 pins={'PioneerNativeBfpModelBandData.h':'d3ff8eeb5830fb33f908a00d1d9dc17860724d41aedc2f0e0bf0045dd8deaae3',
       'PioneerNativeBfpModelBandMetadata.h':'664591584043b713148464b16054ba25d3456bcd603ad34686dc7208535d0fd9'}
def samples():
 for name,pin in pins.items():
  path=private/name;st=path.lstat()
  assert stat.S_ISREG(st.st_mode) and st.st_uid==os.geteuid() and st.st_nlink==1 and stat.S_IMODE(st.st_mode)==0o600
  assert hashlib.sha256(path.read_bytes()).hexdigest()==pin
samples()
here=Path(__file__).resolve().parent;root=here.parent
files=[root/name for name in ('PioneerQ38Bfp8WorkerJob.h','PioneerQ38Bfp8WorkerLoader.h','PioneerQ38Bfp8WorkerCompute.h','PioneerQ38Qgw8Bfp8ReuseRole.h',
 'PioneerQ38Bfp8WorkerConsumer.h','PioneerQ38HybridJobBinding.h','PioneerQ38HybridModelPlan.h',
 'blackhole_q38_bfp8_worker_leaf.cpp','blackhole_q38_bfp8_worker_start.S','build_bfp8_worker_leaf.py')]
files += [here/name for name in ('pioneer_q38_bfp8_worker_job_test.cpp','pioneer_q38_bfp4_worker_loader_test.cpp',
 'pioneer_q38_bfp8_worker_consumer_test.cpp','pioneer_q38_bfp8_worker_leaf_image_test.cpp',
 'pioneer_q38_bfp8_worker_private_image_test.cpp','pioneer_q38_hybrid_job_binding_test.cpp')]
before={path:hashlib.sha256(path.read_bytes()).hexdigest() for path in files}
with tempfile.TemporaryDirectory(prefix='pioneer-bfp8-worker-suite-',dir='/private/tmp') as temporary:
 temp=Path(temporary);image=temp/'image'
 subprocess.run([sys.executable,str(root/'build_bfp8_worker_leaf.py'),str(build),str(image),*(['--reuse-input'] if reuse else []),
                 *(['--watch-incoming'] if watch else []),*(['--batch-input'] if batch else [])],check=True)
 tool=str(build/'cross-tools-riscv64/bin/riscv64-unknown-haiku-')
 assert not subprocess.check_output([tool+'nm','-u',str(image/'program.elf')],text=True).strip()
 if batch:
  assert hashlib.sha256((image/'text.bin').read_bytes()).hexdigest()=='77d87adaf6ab3f9c34fc9f1c50c627d6174676b700f24bca34c8a28f0bcd1dd0'
  assert not re.search(r'\b(?:sb|ebreak|divu?|remu?|mulh[su]*)\b',(image/'program.disassembly').read_text())
  refused=temp/'refused-combination'
  result=subprocess.run([sys.executable,str(root/'build_bfp8_worker_leaf.py'),str(build),str(refused),'--batch-input','--reuse-input'],capture_output=True)
  assert result.returncode and not refused.exists()
 compiler=os.environ.get('CXX','c++')
 cases=(('job','pioneer_q38_bfp8_worker_job_test.cpp',[]),('loader','pioneer_q38_bfp4_worker_loader_test.cpp',['-DPIONEER_TEST_BFP8=1']),
  ('consumer','pioneer_q38_bfp8_worker_consumer_test.cpp',[]),('image','pioneer_q38_bfp8_worker_leaf_image_test.cpp',[]),
  ('private','pioneer_q38_bfp8_worker_private_image_test.cpp',['-I'+str(private)]),('binding','pioneer_q38_hybrid_job_binding_test.cpp',[]))
 for name,source,flags in cases:
  binary=temp/('test-'+name)
  subprocess.run([compiler,'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-ffp-contract=off',
   '-fsanitize=address,undefined',*flags,*(['-DPIONEER_Q38_BFP8_INPUT_REUSE_EXPERIMENT=1'] if reuse and name in ('consumer','image','private') else []),
   *(['-DPIONEER_Q38_BFP8_BATCH_INPUT_EXPERIMENT=1'] if batch and name in ('loader','consumer','image','private') else []),
   *(['-DPIONEER_Q38_BFP8_INCOMING_WATCH_EXPERIMENT=1'] if watch and name in ('consumer','image','private') else []),
   str(here/source),'-o',str(binary)],check=True)
  env=dict(os.environ,ASAN_OPTIONS='quarantine_size_mb=32',UBSAN_OPTIONS='halt_on_error=1')
  subprocess.run([str(binary),*([str(image)] if name in ('image','private') else [])],check=True,env=env)
 for selection in ('_KERNEL_MODE','PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE','PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_FULL_MODEL_CPU_REFERENCE','PIONEER_Q38_BFP4_WORKER_DIAGNOSTIC'):
  result=subprocess.run([compiler,'-std=c++17','-DPIONEER_TEST_BFP8=1','-D'+selection,'-E',str(here/'pioneer_q38_bfp4_worker_loader_test.cpp')],
   stdout=subprocess.DEVNULL,stderr=subprocess.PIPE,text=True)
  assert result.returncode and 'BFP8 worker loader is an isolated offboard diagnostic' in result.stderr,selection
 if reuse:
  for selection in ('PIONEER_Q38_HYBRID_WORKER_IMAGE','PIONEER_Q38_GLOBAL_WORKER_IMAGE','PIONEER_Q38_BF16_WORKER_IMAGE',
                    'PIONEER_DRIVER_BUILD_TAG','PIONEER_Q38_HEAD_MATRIX_WORKER_INSPECTION','PIONEER_Q38_HEAD_MATRIX_ROOT_INSPECTION',
                    'PIONEER_Q38_TOKEN_NATIVE_CONTEXT','PIONEER_Q38_FULL_OFFLOAD_CLIENT','PIONEER_Q38_FULL_OFFLOAD_NATIVE',
                    'PIONEER_Q38_HYBRID_FULL_CLIENT','PIONEER_Q38_HYBRID_FULL_NATIVE','PIONEER_Q38_QGW8_NATIVE_INSPECTION'):
   result=subprocess.run([compiler,'-std=c++17','-DPIONEER_TEST_BFP8=1','-DPIONEER_Q38_BFP8_INPUT_REUSE_EXPERIMENT=1','-D'+selection,
    '-E',str(here/'pioneer_q38_bfp4_worker_loader_test.cpp')],stdout=subprocess.DEVNULL,stderr=subprocess.PIPE,text=True)
   assert result.returncode and 'BFP8 input reuse requires its isolated leaf' in result.stderr,selection
  result=subprocess.run([compiler,'-std=c++17','-DPIONEER_TEST_BFP8=1','-DPIONEER_Q38_BFP8_INPUT_REUSE_EXPERIMENT=0',
   '-E',str(here/'pioneer_q38_bfp4_worker_loader_test.cpp')],stdout=subprocess.DEVNULL,stderr=subprocess.PIPE,text=True)
  assert result.returncode and 'BFP8 input reuse requires its isolated leaf' in result.stderr
 if batch:
  for selection in ('_KERNEL_MODE','PIONEER_DRIVER_BUILD_TAG','TT_PIONEER_DRIVER_RELOAD','PIONEER_Q38_HYBRID_WORKER_IMAGE',
                    'PIONEER_Q38_GLOBAL_WORKER_IMAGE','PIONEER_Q38_BF16_WORKER_IMAGE',
                    'PIONEER_Q38_HEAD_MATRIX_WORKER_INSPECTION','PIONEER_Q38_HEAD_MATRIX_ROOT_INSPECTION',
                    'PIONEER_Q38_TOKEN_NATIVE_CONTEXT','PIONEER_Q38_QGW8_NATIVE_INSPECTION',
                    'PIONEER_Q38_HYBRID_FULL_NATIVE','PIONEER_Q38_HYBRID_FULL_CLIENT','PIONEER_Q38_FULL_OFFLOAD_NATIVE',
                    'PIONEER_Q38_FULL_OFFLOAD_CLIENT','PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE',
                    'PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_FULL_MODEL_CPU_REFERENCE','PIONEER_Q38_BFP8_INPUT_REUSE_EXPERIMENT'):
   result=subprocess.run([compiler,'-std=c++17','-DPIONEER_TEST_BFP8=1','-DPIONEER_Q38_BFP8_BATCH_INPUT_EXPERIMENT=1',
                          '-D'+selection+'=1','-E',str(here/'pioneer_q38_bfp4_worker_loader_test.cpp')],
                         stdout=subprocess.DEVNULL,stderr=subprocess.PIPE,text=True)
   assert result.returncode and 'BFP8 batching requires an isolated leaf or worker experiment' in result.stderr,selection
  result=subprocess.run([compiler,'-std=c++17','-DPIONEER_TEST_BFP8=1','-DPIONEER_Q38_BFP8_BATCH_INPUT_EXPERIMENT=0',
                         '-E',str(here/'pioneer_q38_bfp4_worker_loader_test.cpp')],
                        stdout=subprocess.DEVNULL,stderr=subprocess.PIPE,text=True)
  assert result.returncode and 'BFP8 batching requires an isolated leaf or worker experiment' in result.stderr
  loader='#include "'+str(root/'PioneerQ38Bfp8WorkerLoader.h')+'"\n'
  for defs,body in (([],loader+'#define PIONEER_Q38_BFP8_BATCH_INPUT_EXPERIMENT 1\n'+loader),
                    (['-DPIONEER_Q38_BFP8_BATCH_INPUT_EXPERIMENT=1'],loader+'#undef PIONEER_Q38_BFP8_BATCH_INPUT_EXPERIMENT\n'+loader)):
   result=subprocess.run([compiler,'-std=c++17','-DPIONEER_Q38_BFP8_WORKER_DIAGNOSTIC=1',*defs,'-fsyntax-only','-x','c++','-'],
                         input=body,text=True,capture_output=True)
   assert result.returncode and 'late batched-input' in result.stderr
 if watch:
  for flags in (['-DPIONEER_Q38_BFP8_INCOMING_WATCH_EXPERIMENT=1'],
                ['-DPIONEER_Q38_BFP8_INPUT_REUSE_EXPERIMENT=1','-DPIONEER_Q38_BFP8_INCOMING_WATCH_EXPERIMENT=0']):
   result=subprocess.run([compiler,'-std=c++17',*flags,'-E',str(here/'pioneer_q38_bfp8_worker_consumer_test.cpp')],
    stdout=subprocess.DEVNULL,stderr=subprocess.PIPE,text=True)
   assert result.returncode and 'Incoming write watch requires the isolated BFP8 input-reuse experiment' in result.stderr
 for selection in ('_KERNEL_MODE','PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE','PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_FULL_MODEL_CPU_REFERENCE'):
  result=subprocess.run([compiler,'-std=c++17','-D'+selection,'-E',str(here/'pioneer_q38_hybrid_job_binding_test.cpp')],
   stdout=subprocess.DEVNULL,stderr=subprocess.PIPE,text=True)
  assert result.returncode and 'Hybrid job binding is offline diagnostic data' in result.stderr,selection
samples()
for path,digest in before.items():assert hashlib.sha256(path.read_bytes()).hexdigest()==digest,path
print('PASS BFP8 worker and hybrid binding suite'+(' with input reuse' if reuse else '')+(' with per-band input batching' if batch else '')+(' and incoming watch' if watch else '')+': actual RV32 simulated, 3 private real bands, role refusals; NO DEVICE OR WORK')
