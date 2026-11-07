#!/usr/bin/env python3
"""Verify QGW7 packaging and non-loadable native selection, never a device."""
import hashlib
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile

assert __debug__ and len(sys.argv)==5 and not os.environ.get('PYTHONOPTIMIZE')
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
sdk,root_image,worker,old_header=map(Path,sys.argv[1:])
assert all(p.is_absolute() for p in (sdk,root_image,worker,old_header))
assert sdk.is_dir() and root_image.is_dir() and worker.is_dir() and old_header.is_file()
assert hashlib.sha256(old_header.read_bytes()).hexdigest()=='2598f7a469ddd843ab1bbe732d0960dacaed819b4710b507430274b46c874f49'
here=Path(__file__).resolve().parent;tools=here.parent;sys.path.insert(0,str(tools))
from PioneerQ38HybridMatrixProgramPins import ROOT_PINS, WORKER_PINS
compiler=os.environ.get('CXX','c++')
def run(*args):subprocess.run([str(a) for a in args],check=True)
with tempfile.TemporaryDirectory(prefix='pioneer-qgw7-native-',dir='/private/tmp') as name:
 tmp=Path(name);header=tmp/'matrix.h';again=tmp/'matrix-again.h'
 gen=tools/'generate_qwen38_hybrid_programs.py'
 for target in (header,again):run(sys.executable,gen,'--matrix-attention-inspection',root_image,worker,target)
 data=header.read_bytes();assert data==again.read_bytes()
 assert hashlib.sha256(data).hexdigest()=='5dd198d4676599329fd59626a8924ef2f8fbe4a905883a471f6294fa89efe7b7'
 refused=subprocess.run([sys.executable,str(gen),'--matrix-attention-inspection',str(root_image),str(worker),str(header)],capture_output=True)
 assert refused.returncode and header.read_bytes()==data
 objcopy=sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku-objcopy'
 for folder,pins in ((root_image,ROOT_PINS),(worker,WORKER_PINS)):
  for filename,size,digest in pins:
   original=(folder/filename).read_bytes();assert len(original)==size and hashlib.sha256(original).hexdigest()==digest
   if filename.endswith('.bin'):
    extracted=tmp/(folder.name+'-'+filename)
    run(objcopy,'-O','binary','-j','.'+filename[:-4],folder/'program.elf',extracted)
    assert extracted.read_bytes()==original
 for folder,pins,filename in ((root_image,ROOT_PINS,'program.elf'),(worker,WORKER_PINS,'dense.bin')):
  bad=tmp/('bad-'+folder.name);bad.mkdir()
  for item,_,_ in pins:shutil.copyfile(folder/item,bad/item)
  with (bad/filename).open('r+b') as f:
   original=f.read(1);assert original;f.seek(0);f.write(bytes([original[0]^1]))
  target=tmp/('refused-'+folder.name+'.h')
  r,w=(bad,worker) if folder==root_image else (root_image,bad)
  refused=subprocess.run([sys.executable,str(gen),'--matrix-attention-inspection',str(r),str(w),str(target)],capture_output=True)
  assert refused.returncode and not target.exists()
 flags=['-DPIONEER_Q38_TOKEN_BF16_PROGRAMS=1','-DPIONEER_Q38_HYBRID_JOURNAL_DIAGNOSTIC=1',
  '-DPIONEER_Q38_HYBRID_PROGRAMS_DIAGNOSTIC=1','-DPIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION=1',
  '-DPIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT=1']
 def compile_test(source,output,chosen,extra=()):
  run(compiler,'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',
   '-I'+str(tools),*flags,'-DPIONEER_Q38_HYBRID_PROGRAMS_HEADER="'+str(chosen)+'"',*extra,source,'-o',output)
 emitted=tmp/'emitted';compile_test(here/'pioneer_q38_hybrid_programs_test.cpp',emitted,header)
 run(emitted,root_image,worker)
 selection=here/'pioneer_q38_hybrid_program_selection_test.cpp';selected=tmp/'selected'
 compile_test(selection,selected,header);run(selected)
 # New and old grammars must never silently accept each other's bytecode.
 for chosen,selectors in ((old_header,flags),(header,flags[:-2]),(header,flags[:-1])):
  result=subprocess.run([compiler,'-std=c++17','-I'+str(tools),*selectors,
   '-DPIONEER_Q38_HYBRID_PROGRAMS_HEADER="'+str(chosen)+'"','-fsyntax-only',str(selection)],capture_output=True)
  assert result.returncode,('mismatched program/grammar accepted',chosen,selectors)
 for forbidden in ('PIONEER_Q38_HYBRID_FULL_NATIVE','PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE'):
  result=subprocess.run([compiler,'-std=c++17','-I'+str(tools),*flags,'-D'+forbidden+'=1',
   '-DPIONEER_Q38_HYBRID_PROGRAMS_HEADER="'+str(header)+'"','-fsyntax-only',str(selection)],capture_output=True)
  assert result.returncode,('unsafe matrix role accepted',forbidden)
 # Test against the complete valid physical role, not just a deliberately
 # incomplete combination which might fail for an unrelated missing parent.
 driver=tools.parent.parent/'src/add-ons/kernel/drivers/misc'
 declared=re.findall(r'^#define ([A-Z][A-Z0-9_]*) 1$',(driver/'pioneer_q38_hybrid_full_driver.cpp').read_text(),re.M)
 physical=['-D_KERNEL_MODE=1',*['-D'+name+'=1' for name in declared],
  *['-DTT_PIONEER_EXPERIMENTAL_Q38_'+name+'=1' for name in
    ('BANKS','PANEL','AFFINE','BLOCK','STREAM','ROWS','COHORT','COHORT_SEALED','COHORT_PAYLOAD','DISPATCH','DISPATCH_VECTOR')],
  '-DTT_PIONEER_DRIVER_RELOAD=1','-DPIONEER_DRIVER_BUILD_TAG=0x2026110700010126ULL','-DPIONEER_Q38_HYBRID_FIXED_126=1']
 role=driver/'PioneerQ38HybridFullNativeRole.h'
 okay=subprocess.run([compiler,'-std=c++17',*physical,'-E','-x','c++',str(role)],capture_output=True)
 assert okay.returncode==0,okay.stderr.decode()[:1000]
 refused=subprocess.run([compiler,'-std=c++17',*physical,*flags[-2:],'-E','-x','c++',str(role)],capture_output=True)
 assert refused.returncode and b'Hybrid full native requires' in refused.stderr
 run(sys.executable,here/'run_pioneer_q38_token_native_binding_tests.py','--hybrid-matrix-inspection',header)
 print('PASS QGW7 authenticated image packaging, all 120 code maps, identity/role refusals and non-loadable native binding; no device or TPS')
