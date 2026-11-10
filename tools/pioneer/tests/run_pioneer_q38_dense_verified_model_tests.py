#!/usr/bin/env python3
"""QGWD verified-model role/early refusal; synthetic, no model/device."""
import os
from pathlib import Path
import subprocess
import tempfile
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
here=Path(__file__).resolve().parent;tools=here.parent
compiler=os.environ.get('CXX','c++');fingerprint='0123456789abcdef'*4
base=[compiler,'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-Wno-misleading-indentation','-ffp-contract=off']
roles=['-DPIONEER_Q38_DENSE_Q8_VERIFIED_MODEL_INSPECTION=1','-DPIONEER_Q38_DENSE_Q8_MATRIX_PLAN_INSPECTION=1',
 '-DPIONEER_Q38_BFP4_PREPARATION_DIAGNOSTIC=1','-DPIONEER_Q38_HEAD_BLOCK_FLOAT_DIAGNOSTIC=1',
 '-DPIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION=1','-DPIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION=1',
 '-DPIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT=1','-DPIONEER_BFP4_IMPLEMENTATION_SHA256="'+fingerprint+'"',
 '-DPIONEER_HEAD_BFP8_IMPLEMENTATION_SHA256="'+fingerprint+'"']
source=str(here/'pioneer_q38_dense_verified_model_test.cpp')
for forbidden in ('_KERNEL_MODE','PIONEER_DRIVER_BUILD_TAG','PIONEER_Q38_TOKEN_NATIVE_CONTEXT',
 'PIONEER_Q38_FULL_OFFLOAD_NATIVE','PIONEER_Q38_FULL_OFFLOAD_CLIENT','PIONEER_Q38_WORK_INFERENCE',
 'TT_PIONEER_Q38_WORK_MODE','PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_FULL_MODEL_CPU_REFERENCE'):
 result=subprocess.run([*base,*roles,'-D'+forbidden+'=1','-fsyntax-only',source],capture_output=True,text=True)
 assert result.returncode and 'isolated offline preparation only' in result.stderr,forbidden
with tempfile.TemporaryDirectory(prefix='qgwd-verified-model-',dir='/private/tmp') as tmp:
 binary=Path(tmp)/'test'
 subprocess.run([*base,*roles,'-fsanitize=address,undefined',source,str(tools/'PioneerModelWeightCache.cpp'),'-o',str(binary)],check=True)
 subprocess.run([str(binary)],check=True,env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
 cli=Path(tmp)/'preflight'
 subprocess.run([*base,*roles,str(tools/'pioneer_q38_qgwd_cache_preflight.cpp'),str(tools/'PioneerModelWeightCache.cpp'),'-o',str(cli)],check=True)
 for args in ([],['--wrong','/a','/b','/c','/d'],['--verify-qgwd','relative','/b','/c','/d']):
  assert subprocess.run([str(cli),*args],capture_output=True).returncode==2,args
 missing=[str(Path(tmp)/name) for name in ('missing4','missing8','missinghead','missingdense')]
 assert subprocess.run([str(cli),'--verify-qgwd',*missing],capture_output=True).returncode==1
 assert not any(Path(path).exists() for path in missing)
print('PASS QGWD four-cache verified-model early refusal and isolated-role checks; no model/device')
