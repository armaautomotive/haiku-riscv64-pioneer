#!/usr/bin/env python3
"""Synthetic Q4 fixture mapping and strict host-only role; no card or model."""
import os
from pathlib import Path
import subprocess
import tempfile

assert __debug__ and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
here=Path(__file__).resolve().parent;compiler=os.environ.get('CXX','c++')
source=here/'pioneer_q38_synthetic_embedding_bridge.cpp';fixture=here/'synthetic_q4_embedding.fixture'
assert fixture.read_bytes()==b'SYNTHETIC_Q4K_EMBEDDING_V1\n'
for flags in ([],['-DPIONEER_Q38_SYNTHETIC_EMBEDDING_TEST=0'],
              *(['-DPIONEER_Q38_SYNTHETIC_EMBEDDING_TEST=1','-D'+role+'=1'] for role in (
               '_KERNEL_MODE','PIONEER_Q38_TOKEN_NATIVE_CONTEXT','PIONEER_Q38_WORK_INFERENCE',
               'TT_PIONEER_Q38_WORK_MODE','PIONEER_Q38_FULL_OFFLOAD_NATIVE','PIONEER_Q38_FULL_OFFLOAD_CLIENT',
               'PIONEER_Q38_BF16_FULL_NATIVE','PIONEER_Q38_BF16_FULL_CLIENT',
               'PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE','PIONEER_Q38_GLOBAL_WORKER_IMAGE'))):
 result=subprocess.run([compiler,'-std=c++17',*flags,'-E',str(source)],capture_output=True,text=True)
 assert result.returncode and 'Synthetic embedding bytes require their isolated host-test implementation' in result.stderr,flags
with tempfile.TemporaryDirectory(prefix='pioneer-synthetic-embedding-') as directory:
 binary=Path(directory)/'test'
 subprocess.run([compiler,'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',
  '-DPIONEER_Q38_SYNTHETIC_EMBEDDING_TEST=1',str(source),str(here/'pioneer_q38_synthetic_embedding_bridge_test.cpp'),
  '-o',str(binary)],check=True)
 subprocess.run([str(binary),str(fixture)],check=True)
print('PASS synthetic Q4 byte mapping, refusal/revocation and host-only role; NO model or device')
