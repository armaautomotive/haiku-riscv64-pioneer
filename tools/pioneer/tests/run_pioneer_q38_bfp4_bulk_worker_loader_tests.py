#!/usr/bin/env python3
"""Host-only bulk loader byte equivalence and role-exclusion checks."""
import os
import hashlib
from pathlib import Path
import re
import stat
import subprocess
import sys
import tempfile

here=Path(__file__).resolve().parent;root=here.parent;compiler=os.environ.get('CXX','c++')
assert len(sys.argv)<=3
sdk=Path(sys.argv[1]) if len(sys.argv)>=2 else None
private=Path(sys.argv[2]) if len(sys.argv)==3 else None
assert sdk is None or (sdk.is_absolute() and sdk.is_dir())
assert private is None or (private.is_absolute() and not private.is_symlink() and private.is_dir()
 and private.stat().st_uid==os.geteuid() and stat.S_IMODE(private.stat().st_mode)==0o700)
pins=('edbc6d7cadcd85d0114ff16a5c44afb3627296497a7a0e03f76efa41da5ac7dd',
 '59f05959ef8486021eefd0b7e02e083b6f18a7eef747f15d5e24e6e87c587695',
 '036bcee72fc9bb84fedb9c6f93a6d22358f9d7c90c610cc5c19159f002a587e3')
def samples():
 if private:
  for i,pin in enumerate(pins):
   path=private/('band%d.rbp4'%i);info=path.lstat()
   assert stat.S_ISREG(info.st_mode) and info.st_uid==os.geteuid() and info.st_nlink==1
   assert stat.S_IMODE(info.st_mode)==0o600 and hashlib.sha256(path.read_bytes()).hexdigest()==pin
samples()
with tempfile.TemporaryDirectory(prefix='q38-bfp4-bulk-',dir='/private/tmp') as directory:
 binary=Path(directory)/'bulk-test';source=here/'pioneer_q38_bfp4_bulk_worker_loader_test.cpp'
 subprocess.run([compiler,'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-Wno-unused-function',
  '-fsanitize=address,undefined',str(source),'-o',str(binary)],check=True)
 subprocess.run([str(binary)],check=True)
 header=root/'PioneerQ38Bfp4BulkWorkerLoader.h'
 for forbidden in ('_KERNEL_MODE','PIONEER_DRIVER_BUILD_TAG','PIONEER_Q38_HYBRID_WORKER_IMAGE',
                   '__FAST_MATH__','PIONEER_Q38_BFP4_INPUT_REUSE_EXPERIMENT',
                   'PIONEER_Q38_BFP4_INPUT_REUSE_CHECKSUM_EXPERIMENT','PIONEER_Q38_HYBRID_FULL_CLIENT',
                   'PIONEER_Q38_HYBRID_FULL_NATIVE','PIONEER_Q38_FULL_OFFLOAD_CLIENT',
                   'PIONEER_Q38_FULL_OFFLOAD_NATIVE','PIONEER_Q38_BF16_FULL_CLIENT','PIONEER_Q38_BF16_FULL_NATIVE',
                   'PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE',
                   'PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_FULL_MODEL_CPU_REFERENCE'):
  result=subprocess.run([compiler,'-std=c++17','-DPIONEER_Q38_BFP4_WORKER_DIAGNOSTIC=1',
   '-DPIONEER_Q38_BFP4_BULK_LOAD_EXPERIMENT=1','-D'+forbidden+'=1','-E','-x','c++',str(header)],
   capture_output=True,text=True)
  assert result.returncode and 'BFP4 bulk loading is an isolated offline experiment' in result.stderr,forbidden
 for missing in ([],['-DPIONEER_Q38_BFP4_BULK_LOAD_EXPERIMENT=0']):
  result=subprocess.run([compiler,'-std=c++17','-DPIONEER_Q38_BFP4_WORKER_DIAGNOSTIC=1',*missing,
   '-E','-x','c++',str(header)],capture_output=True,text=True)
  assert result.returncode and 'BFP4 bulk loading is an isolated offline experiment' in result.stderr
 if sdk:
  steps={}
  for mode in ('normal','bulk'):
   image=Path(directory)/('image-'+mode);executable=Path(directory)/('machine-'+mode)
   command=[sys.executable,str(root/'build_bfp4_worker_leaf.py'),str(sdk),str(image)]
   if mode=='bulk':command+=['--bulk-load']
   subprocess.run(command,check=True)
   if mode=='normal':
    assert hashlib.sha256((image/'text.bin').read_bytes()).hexdigest()=='04d43a4e5ad5b766fea07d0d3f9f48168557abda4772826d804f643db644faaf'
   compile=[compiler,'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-ffp-contract=off',
    '-fsanitize=address,undefined']
   if mode=='bulk':compile+=['-DPIONEER_Q38_BFP4_BULK_LOAD_EXPERIMENT=1']
   compile += [str(here/'pioneer_q38_bfp4_worker_leaf_image_test.cpp'),'-o',str(executable)]
   subprocess.run(compile,check=True)
   output=subprocess.check_output([str(executable),str(image)],text=True);print(output,end='')
   if private:
    for i in range(3):subprocess.run([str(executable),str(image),str(private/('band%d.rbp4'%i))],check=True)
   for width in (5120,17408):
    match=re.search(r'K'+str(width)+r' peer119:.* steps(\d+) stack\d+;',output)
    assert match,(mode,width);steps[mode,width]=int(match.group(1))
  for width in (5120,17408):
   assert steps['bulk',width]>steps['normal',width]
   print('SIMULATED_RV32_STEPS width%d normal%d bulk%d; NOT HARDWARE TIME'%(width,steps['normal',width],steps['bulk',width]))
 samples()
 print('PASS BFP4 bulk transport simulation and role refusals; no driver, card or speed qualification')
