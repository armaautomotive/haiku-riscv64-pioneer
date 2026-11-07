#!/usr/bin/env python3
"""Offline output-head converter checks; no device or model access."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile

assert __debug__ and len(sys.argv)==1 and not os.environ.get('PYTHONOPTIMIZE')
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
here=Path(__file__).resolve().parent;root=here.parent;compiler=os.environ.get('CXX','c++')
role=root/'PioneerQ38HeadBlockFloatDiagnostic.h'
def check(flags,valid):
 result=subprocess.run([compiler,'-std=c++17','-x','c++','-fsyntax-only',*flags,str(role)],capture_output=True)
 assert (result.returncode==0)==valid,result.stderr.decode()[:1000]
required='-DPIONEER_Q38_HEAD_BLOCK_FLOAT_DIAGNOSTIC=1'
check([required],True);check([],False);check(['-DPIONEER_Q38_HEAD_BLOCK_FLOAT_DIAGNOSTIC=0'],False)
for forbidden in ('_KERNEL_MODE','PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE',
                  'PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_FULL_MODEL_CPU_REFERENCE',
                  'PIONEER_Q38_HYBRID_FULL_NATIVE','PIONEER_Q38_TOKEN_NATIVE_CONTEXT',
                  'PIONEER_DRIVER_BUILD_TAG','__FAST_MATH__'):
 check([required,'-D'+forbidden+'=1'],False)
with tempfile.TemporaryDirectory(prefix='pioneer-q38-head-block-float-',dir='/private/tmp') as directory:
 tmp=Path(directory);fixture=tmp/'fixture';fragment=tmp/'fragment';cache=tmp/'cache-test';real=tmp/'real';realPages=tmp/'real-pages';capacity=tmp/'capacity'
 flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-ffp-contract=off','-fsanitize=address,undefined']
 subprocess.run([compiler,*flags,str(here/'pioneer_q38_head_block_float_test.cpp'),'-o',str(fixture)],check=True)
 subprocess.run([str(fixture)],check=True,env={**os.environ,'UBSAN_OPTIONS':'halt_on_error=1'})
 subprocess.run([compiler,*flags,str(here/'pioneer_q38_head_bfp8_fragment_test.cpp'),'-o',str(fragment)],check=True)
 subprocess.run([str(fragment)],check=True,env={**os.environ,'UBSAN_OPTIONS':'halt_on_error=1'})
 subprocess.run([compiler,*flags,str(here/'pioneer_q38_head_bfp8_cache_test.cpp'),str(root/'PioneerModelWeightCache.cpp'),'-o',str(cache)],check=True)
 subprocess.run([str(cache),str(tmp)],check=True,env={**os.environ,'UBSAN_OPTIONS':'halt_on_error=1'})
 subprocess.run([compiler,*flags,str(here/'pioneer_q38_head_block_float_real_diagnostic.cpp'),
  str(root/'PioneerModelWeightCache.cpp'),'-o',str(real)],check=True)
 for args in ([],['relative'],['/private/tmp/nonexistent-pioneer-q38-head-model']):
  assert subprocess.run([str(real),*args],capture_output=True).returncode!=0
 subprocess.run([compiler,*flags,str(here/'pioneer_q38_head_bfp8_real_pages.cpp'),
  str(root/'PioneerModelWeightCache.cpp'),'-o',str(realPages)],check=True)
 for args in ([],['relative'],['/private/tmp/nonexistent-pioneer-q38-head-model']):
  assert subprocess.run([str(realPages),*args],capture_output=True).returncode!=0
 subprocess.run([compiler,*flags,'-Wno-unused-function',str(here/'pioneer_q38_head_block_float_capacity.cpp'),
  '-o',str(capacity)],check=True)
 report=subprocess.check_output([str(capacity)],text=True,env={**os.environ,'UBSAN_OPTIONS':'halt_on_error=1'})
 assert 'RESULT all_bfp8_banks_fit=0;' in report
 for dropped,mask,fit in ((0,'1fff000000000000',0),(1,'1ffe000000000000',0),
                          (2,'1ffc000000000000',1),(3,'1ff8000000000000',1)):
  assert f'ALTERNATIVE dropped_layers={dropped} mask={mask} head_bfp8_same_bank_capacity={fit};' in report
 print(report,end='')
print('PASS isolated head BFP4/BFP8 diagnostic role and argument refusals; no model or device access')
