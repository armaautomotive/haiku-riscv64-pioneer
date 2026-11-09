#!/usr/bin/env python3
"""Authenticate the QGWC program bundle and test its complete code footprint."""
import hashlib
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

assert __debug__ and len(sys.argv)==6 and not os.environ.get('PYTHONOPTIMIZE')
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
sdk,root_a,root_b,worker_a,worker_b=map(Path,sys.argv[1:])
assert all(p.is_absolute() and p.is_dir() for p in (sdk,root_a,root_b,worker_a,worker_b))
here=Path(__file__).resolve().parent;root=here.parent;sys.path.insert(0,str(root))
from PioneerQ38QgwcSrcaProgramPins import ROOT_PINS,WORKER_PINS
generator=root/'generate_qwen38_hybrid_programs.py';compiler=os.environ.get('CXX','c++')
flags=['-DPIONEER_Q38_QGWC_PROGRAMS_INSPECTION=1','-DPIONEER_Q38_QGWC_PLAN_SIMULATION=1',
 '-DPIONEER_Q38_QGW8_LAYOUT_INSPECTION=1',
 '-DPIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION=1','-DPIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION=1',
 '-DPIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT=1']
with tempfile.TemporaryDirectory(prefix='qgwc-programs-',dir='/private/tmp') as directory:
 tmp=Path(directory);first=tmp/'a.h';second=tmp/'b.h'
 for r,w,out in ((root_a,worker_a,first),(root_b,worker_b,second)):
  subprocess.run([sys.executable,str(generator),'--qgwc-srca-inspection',str(r),str(w),str(out)],check=True)
 assert first.read_bytes()==second.read_bytes()
 assert hashlib.sha256(first.read_bytes()).hexdigest()=='fbde4c739bb7bce2c76cb4a1d6ac93c4a083a4de9c559433686e8f036e34a1b0'
 native_a=tmp/'native-a.h';native_b=tmp/'native-b.h'
 for r,w,out in ((root_a,worker_a,native_a),(root_b,worker_b,native_b)):
  subprocess.run([sys.executable,str(generator),'--qgwc-srca-native',str(r),str(w),str(out)],check=True)
 assert native_a.read_bytes()==native_b.read_bytes()
 assert hashlib.sha256(native_a.read_bytes()).hexdigest()=='4e7b07e725198a0a65d5731a07cf3cdf1402b48040ba4cd18ac9242cd5039690'
 assert native_a.read_bytes()!=first.read_bytes()
 before=first.read_bytes()
 result=subprocess.run([sys.executable,str(generator),'--qgwc-srca-inspection',str(root_a),str(worker_a),str(first)],capture_output=True)
 assert result.returncode and first.read_bytes()==before
 for folder,pins,name in ((root_a,ROOT_PINS,'program.elf'),(worker_a,WORKER_PINS,'dense.bin')):
  bad=tmp/('bad-'+folder.name);bad.mkdir()
  for filename,_,_ in pins:shutil.copyfile(folder/filename,bad/filename)
  with (bad/name).open('r+b') as stream:
   value=stream.read(1);stream.seek(0);stream.write(bytes([value[0]^1]))
  out=tmp/('bad-'+folder.name+'.h');r,w=(bad,worker_a) if folder==root_a else (root_a,bad)
  result=subprocess.run([sys.executable,str(generator),'--qgwc-srca-inspection',str(r),str(w),str(out)],capture_output=True)
  assert result.returncode and not out.exists()
 for extra in (['--qgwc-srca-inspection','--qgw8-head-inspection'],['--qgwc-srca-inspection','--qgwc-srca-inspection']):
  out=tmp/'ambiguous.h';result=subprocess.run([sys.executable,str(generator),*extra,str(root_a),str(worker_a),str(out)],capture_output=True)
  assert result.returncode and not out.exists()
 header='-DPIONEER_Q38_HYBRID_PROGRAMS_HEADER="'+str(first)+'"'
 test=tmp/'program-test'
 subprocess.run([compiler,'-std=c++17','-O2','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',
                 '-I'+str(root),header,*flags,str(here/'pioneer_q38_hybrid_programs_test.cpp'),'-o',str(test)],check=True)
 env=dict(os.environ,ASAN_OPTIONS='quarantine_size_mb=32',UBSAN_OPTIONS='halt_on_error=1')
 subprocess.run([str(test),str(root_a),str(worker_a)],check=True,env=env)
 select=['-DPIONEER_Q38_TOKEN_BF16_PROGRAMS=1','-DPIONEER_Q38_HYBRID_JOURNAL_DIAGNOSTIC=1',
         '-DPIONEER_Q38_HYBRID_PROGRAMS_DIAGNOSTIC=1',header,*flags]
 body='''#include <cstdint>
#include "PioneerQ38TokenSelectedPrograms.h"
#include "PioneerQ38SelectedGlobalWorker.h"
static_assert(PioneerQ38TokenPrograms::Family==0x46505451,"QTPF");
static_assert(PioneerQ38SelectedGlobalWorker::Magic==0x43574751,"QGWC");
static_assert(PioneerQ38SelectedGlobalWorker::JobWitness==0x434f4751,"QGOC");
int main(){return PioneerQ38TokenPrograms::CodeSpans(119)==10?0:1;}
'''
 selection=tmp/'selection';subprocess.run([compiler,'-std=c++17','-O2','-Wall','-Wextra','-Werror','-I'+str(root),
                                           *select,'-x','c++','-','-o',str(selection)],input=body,text=True,check=True)
 subprocess.run([str(selection)],check=True)
 for forbidden in ('_KERNEL_MODE','PIONEER_DRIVER_BUILD_TAG','PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE',
                   'PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_FULL_MODEL_CPU_REFERENCE','PIONEER_Q38_QGW8_PROGRAMS_INSPECTION'):
  result=subprocess.run([compiler,'-std=c++17','-I'+str(root),*select,'-D'+forbidden+'=1',
                         '-fsyntax-only','-x','c++','-'],input=body,text=True,capture_output=True)
  assert result.returncode,forbidden
print('PASS QGWC immutable 15-section bundle, exact 120-worker journal footprint, selector and corruption refusals; NO hardware')
