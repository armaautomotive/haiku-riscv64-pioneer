#!/usr/bin/env python3
"""Offboard canonical job, loader, returning leaf and actual RV32 tests."""
import hashlib
import os
from pathlib import Path
import stat
import subprocess
import sys
import tempfile

args=sys.argv[1:];reuse='--reuse-input' in args
if reuse:
 assert args.count('--reuse-input')==1;args.remove('--reuse-input')
assert __debug__ and len(args)==2
build,private=map(Path,args);assert build.is_absolute() and build.is_dir()
assert private.is_absolute() and not private.is_symlink() and stat.S_IMODE(private.stat().st_mode)==0o700
assert private.stat().st_uid==os.geteuid() and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
pins=('edbc6d7cadcd85d0114ff16a5c44afb3627296497a7a0e03f76efa41da5ac7dd',
      '59f05959ef8486021eefd0b7e02e083b6f18a7eef747f15d5e24e6e87c587695',
      '036bcee72fc9bb84fedb9c6f93a6d22358f9d7c90c610cc5c19159f002a587e3')
def samples():
 for i,pin in enumerate(pins):
  path=private/f'band{i}.rbp4';st=path.lstat()
  assert stat.S_ISREG(st.st_mode) and st.st_uid==os.geteuid() and st.st_nlink==1 and stat.S_IMODE(st.st_mode)==0o600
  assert hashlib.sha256(path.read_bytes()).hexdigest()==pin
samples()
here=Path(__file__).resolve().parent;root=here.parent
files=[root/name for name in ('PioneerQ38Bfp4WorkerJob.h','PioneerQ38Bfp4JobBinding.h','PioneerQ38Bfp4WorkerLoader.h',
 'PioneerQ38Bfp4WorkerCompute.h','PioneerQ38Bfp4WorkerConsumer.h','blackhole_q38_bfp4_worker_leaf.cpp','build_bfp4_worker_leaf.py')]
files += [here/name for name in ('pioneer_q38_bfp4_worker_job_test.cpp','pioneer_q38_bfp4_worker_loader_test.cpp',
 'pioneer_q38_bfp4_worker_consumer_test.cpp','pioneer_q38_bfp4_worker_leaf_image_test.cpp',
 'pioneer_q38_native_bfp4_band_numeric_test.cpp')]
before={path:hashlib.sha256(path.read_bytes()).hexdigest() for path in files}
with tempfile.TemporaryDirectory(prefix='pioneer-bfp4-worker-suite-',dir='/private/tmp') as temporary:
 temp=Path(temporary);image=temp/'image'
 subprocess.run([sys.executable,str(root/'build_bfp4_worker_leaf.py'),str(build),str(image),*(['--reuse-input'] if reuse else [])],check=True)
 tool=str(build/'cross-tools-riscv64/bin/riscv64-unknown-haiku-')
 assert not subprocess.check_output([tool+'nm','-u',str(image/'program.elf')],text=True).strip()
 compiler=os.environ.get('CXX','c++')
 for name in ('job','loader','consumer','leaf_image'):
  binary=temp/name;source=here/f'pioneer_q38_bfp4_worker_{name}_test.cpp'
  subprocess.run([compiler,'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-ffp-contract=off',
                  '-fsanitize=address,undefined',*(['-DPIONEER_Q38_BFP4_INPUT_REUSE_EXPERIMENT=1'] if reuse and name=='leaf_image' else []),
                  str(source),'-o',str(binary)],check=True)
  env=dict(os.environ,ASAN_OPTIONS='quarantine_size_mb=32',UBSAN_OPTIONS='halt_on_error=1')
  subprocess.run([str(binary),*([str(image)] if name=='leaf_image' else [])],check=True,env=env)
  if name=='leaf_image':
   for i in range(3):subprocess.run([str(binary),str(image),str(private/f'band{i}.rbp4')],check=True,env=env)
 for selection in ('_KERNEL_MODE','PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE','PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_FULL_MODEL_CPU_REFERENCE'):
  result=subprocess.run([compiler,'-std=c++17','-D'+selection,'-E',str(here/'pioneer_q38_bfp4_worker_loader_test.cpp')],
                        stdout=subprocess.DEVNULL,stderr=subprocess.PIPE,text=True)
  assert result.returncode and 'BFP4 worker loader is an isolated offboard diagnostic' in result.stderr,selection
  result=subprocess.run([compiler,'-std=c++17','-D'+selection,'-E',str(here/'pioneer_q38_bfp4_worker_job_test.cpp')],
                        stdout=subprocess.DEVNULL,stderr=subprocess.PIPE,text=True)
  assert result.returncode and 'BFP4 job binding is offline diagnostic data' in result.stderr,selection
 if reuse:
  for selection in ('PIONEER_Q38_HYBRID_WORKER_IMAGE','PIONEER_DRIVER_BUILD_TAG'):
   result=subprocess.run([compiler,'-std=c++17','-DPIONEER_Q38_BFP4_INPUT_REUSE_EXPERIMENT=1','-D'+selection,
                         '-E',str(here/'pioneer_q38_bfp4_worker_loader_test.cpp')],stdout=subprocess.DEVNULL,stderr=subprocess.PIPE,text=True)
   assert result.returncode and 'Input reuse requires its explicit isolated leaf experiment' in result.stderr,selection
samples()
for path,digest in before.items():assert hashlib.sha256(path.read_bytes()).hexdigest()==digest,path
print('PASS dynamic BFP4 worker suite'+(' (input-reuse experiment)' if reuse else '')+': real RV32, 3 private bands, canonical coverage, role refusals; NO DEVICE, native ownership, Work or full-model speed proof')
