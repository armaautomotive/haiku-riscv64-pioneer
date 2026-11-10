#!/usr/bin/env python3
"""Pin the isolated QGWD ROOT/worker bundle and its larger journal."""
import hashlib
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

args=sys.argv[1:];seal_loop='--qgwd-seal-loop' in args
if seal_loop:
    assert args.count('--qgwd-seal-loop')==1;args.remove('--qgwd-seal-loop')
assert __debug__ and len(args)==5 and not os.environ.get('PYTHONOPTIMIZE')
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
sdk,root_a,root_b,worker_a,worker_b=map(Path,args)
assert all(p.is_absolute() and p.is_dir() for p in (sdk,root_a,root_b,worker_a,worker_b))
here=Path(__file__).resolve().parent;root=here.parent;sys.path.insert(0,str(root))
from PioneerQ38QgwdDenseProgramPins import ROOT_PINS,WORKER_PINS
generator=root/'generate_qwen38_hybrid_programs.py';compiler=os.environ.get('CXX','c++')
flags=['-DPIONEER_Q38_QGWD_PROGRAMS_INSPECTION=1','-DPIONEER_Q38_QGWD_PLAN_SIMULATION=1',
       '-DPIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION=1','-DPIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION=1',
       '-DPIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT=1','-DPIONEER_Q38_DENSE_Q8_MATRIX_FOOTPRINT_INSPECTION=1',
       '-DPIONEER_Q38_DENSE_Q8_MATRIX_WORKER_INSPECTION=1']
if seal_loop:flags+=['-DPIONEER_Q38_QGWD_SEAL_LOOP_INSPECTION=1']
selector='--qgwd-seal-loop-inspection' if seal_loop else '--qgwd-dense-q8-inspection'
with tempfile.TemporaryDirectory(prefix='qgwd-programs-',dir='/private/tmp') as directory:
    tmp=Path(directory);first=tmp/'a.h';second=tmp/'b.h'
    for r,w,out in ((root_a,worker_a,first),(root_b,worker_b,second)):
        subprocess.run([sys.executable,str(generator),selector,str(r),str(w),str(out)],check=True)
    assert first.read_bytes()==second.read_bytes()
    assert hashlib.sha256(first.read_bytes()).hexdigest()==('56b5d7479610c1862b6d6a5642b4441e3dab25d4bcefdcf00224c58efb675a7e' if seal_loop else '184f10919fb20dd04489a64115022f924cf81c270c318c7da2e0cf78491fc328')
    before=first.read_bytes()
    result=subprocess.run([sys.executable,str(generator),selector,str(root_a),str(worker_a),str(first)],capture_output=True)
    assert result.returncode and first.read_bytes()==before
    for folder,pins,name in ((root_a,ROOT_PINS,'program.elf'),(worker_a,WORKER_PINS,'globalworker.bin')):
        bad=tmp/('bad-'+folder.name);bad.mkdir()
        for filename,_,_ in pins:shutil.copyfile(folder/filename,bad/filename)
        with (bad/name).open('r+b') as stream:
            value=stream.read(1);stream.seek(0);stream.write(bytes([value[0]^1]))
        out=tmp/('bad-'+folder.name+'.h');r,w=(bad,worker_a) if folder==root_a else (root_a,bad)
        result=subprocess.run([sys.executable,str(generator),selector,str(r),str(w),str(out)],capture_output=True)
        assert result.returncode and not out.exists()
    for extra in ([selector,'--qgwc-srca-inspection'],[selector]*2, [selector,'--qgwd-dense-q8-native-candidate']):
        out=tmp/'ambiguous.h';result=subprocess.run([sys.executable,str(generator),*extra,str(root_a),str(worker_a),str(out)],capture_output=True)
        assert result.returncode and not out.exists()
    header='-DPIONEER_Q38_HYBRID_PROGRAMS_HEADER="'+str(first)+'"';test=tmp/'program-test'
    subprocess.run([compiler,'-std=c++17','-O2','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',
                    '-I'+str(root),header,*flags,str(here/'pioneer_q38_hybrid_programs_test.cpp'),'-o',str(test)],check=True)
    subprocess.run([str(test),str(root_a),str(worker_a)],check=True,env=dict(os.environ,ASAN_OPTIONS='quarantine_size_mb=32',UBSAN_OPTIONS='halt_on_error=1'))
    select=['-DPIONEER_Q38_TOKEN_BF16_PROGRAMS=1','-DPIONEER_Q38_HYBRID_JOURNAL_DIAGNOSTIC=1',
            '-DPIONEER_Q38_HYBRID_PROGRAMS_DIAGNOSTIC=1',header,*flags]
    body='''#include <cstdint>
#include "PioneerQ38TokenSelectedPrograms.h"
#include "PioneerQ38SelectedGlobalWorker.h"
static_assert(PioneerQ38TokenPrograms::Family==0x47505451,"QTPG");
static_assert(PioneerQ38TokenL1::Family==0x42505451,"expanded journal");
static_assert(PioneerQ38SelectedGlobalWorker::Magic==0x44574751,"QGWD");
int main(){return PioneerQ38TokenPrograms::CodeSpans(119)==10?0:1;}
'''
    selection=tmp/'selection'
    subprocess.run([compiler,'-std=c++17','-O2','-Wall','-Wextra','-Werror','-I'+str(root),*select,
                    '-x','c++','-','-o',str(selection)],input=body,text=True,check=True)
    subprocess.run([str(selection)],check=True)
    for required in ('PIONEER_Q38_QGWD_PROGRAMS_INSPECTION','PIONEER_Q38_QGWD_PLAN_SIMULATION',
                     'PIONEER_Q38_DENSE_Q8_MATRIX_FOOTPRINT_INSPECTION','PIONEER_Q38_DENSE_Q8_MATRIX_WORKER_INSPECTION')+ (('PIONEER_Q38_QGWD_SEAL_LOOP_INSPECTION',) if seal_loop else ()):
        reduced=[flag for flag in select if flag!='-D'+required+'=1']
        result=subprocess.run([compiler,'-std=c++17','-I'+str(root),*reduced,
                               '-fsyntax-only','-x','c++','-'],input=body,text=True,capture_output=True)
        assert result.returncode,required
    for forbidden in ('_KERNEL_MODE','PIONEER_DRIVER_BUILD_TAG','PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE',
                      'PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_FULL_MODEL_CPU_REFERENCE','PIONEER_Q38_QGWC_PROGRAMS_INSPECTION',
                      'PIONEER_Q38_QGW8_PROGRAMS_INSPECTION','PIONEER_Q38_HEAD_MATRIX_ROOT_INSPECTION'):
        result=subprocess.run([compiler,'-std=c++17','-I'+str(root),*select,'-D'+forbidden+'=1',
                               '-fsyntax-only','-x','c++','-'],input=body,text=True,capture_output=True)
        assert result.returncode,forbidden
print('PASS '+('QGWD_SEAL_LOOP' if seal_loop else 'QGWD')+' 15-section bundle and expanded journal, exact role and corruption refusals; NO DEVICE')
