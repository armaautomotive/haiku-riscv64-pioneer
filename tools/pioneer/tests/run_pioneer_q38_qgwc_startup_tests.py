#!/usr/bin/env python3
"""Test QGWC retained startup against the actual linked ROOT; no device."""
import hashlib
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile

assert __debug__ and len(sys.argv)==5 and not os.environ.get('PYTHONOPTIMIZE')
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
sdk,image,worker,header=map(Path,sys.argv[1:])
assert all(p.is_absolute() for p in (sdk,image,worker,header))
assert all(p.is_dir() for p in (sdk,image,worker)) and header.is_file()
for path,digest in ((header,'fbde4c739bb7bce2c76cb4a1d6ac93c4a083a4de9c559433686e8f036e34a1b0'),
                    (image/'program.elf','d44e18940bcd37cf35bc2f124b314843cfd99314e8aaebd5d4983eb074918eb0'),
                    (worker/'program.elf','3450e5b2d46b27e9ac3f81cb62de79d53b0613d85af0acc03a73a149e812e09e')):
 assert hashlib.sha256(path.read_bytes()).hexdigest()==digest,path
here=Path(__file__).resolve().parent;root=here.parent;compiler=os.environ.get('CXX','c++')
flags=['-DPIONEER_Q38_QGWC_PROGRAMS_INSPECTION=1','-DPIONEER_Q38_QGWC_PLAN_SIMULATION=1',
 '-DPIONEER_Q38_TOKEN_BF16_PROGRAMS=1','-DPIONEER_Q38_HYBRID_JOURNAL_DIAGNOSTIC=1',
 '-DPIONEER_Q38_HYBRID_PROGRAMS_DIAGNOSTIC=1','-DPIONEER_Q38_HYBRID_PROGRAMS_HEADER="'+str(header)+'"']
common=[compiler,'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-Wno-unused-function','-Wno-unused-variable',
 '-ffp-contract=off','-fsanitize=address,undefined','-I'+str(root)]
env=dict(os.environ,ASAN_OPTIONS='quarantine_size_mb=32',UBSAN_OPTIONS='halt_on_error=1')
with tempfile.TemporaryDirectory(prefix='qgwc-startup-',dir='/private/tmp') as directory:
 tmp=Path(directory);receipt=tmp/'receipt';helpers=tmp/'helpers';probe=tmp/'root';probe.mkdir()
 subprocess.run([*common,*flags,str(here/'pioneer_q38_qgw8_upload_receipt_test.cpp'),'-o',str(receipt)],check=True)
 result=subprocess.run([str(receipt),'--emit-startup-fixture'],check=True,text=True,capture_output=True,env=env)
 assert 'PASS QGW8 receipt and actual runtime reservation:' in result.stdout
 data={}
 for label,size in (('QGW8_STARTUP_HEAD_FIXTURE',1024),('QGW8_STARTUP_HEAD_JOB',256)):
  found=re.findall(r'^'+label+r' ([0-9a-f]+)$',result.stdout,re.M);assert len(found)==1
  payload=bytes.fromhex(found[0]);assert len(payload)==size;data[label]=tmp/label;data[label].write_bytes(payload)
 assert int.from_bytes(data['QGW8_STARTUP_HEAD_FIXTURE'].read_bytes()[(64+40)*4:(64+41)*4],'little')==0x43574751
 assert int.from_bytes(data['QGW8_STARTUP_HEAD_JOB'].read_bytes()[:4],'little')==0x43574751
 for name in ('bf16.bin','text.bin','terminal.bin','conv.bin'):
  (probe/name).write_bytes((image/name).read_bytes())
 tool=str(sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku-')
 (probe/'program.demangled').write_bytes(subprocess.check_output([tool+'nm','-nC',str(image/'program.elf')]))
 simulation=['-DPIONEER_Q38_QGWC_PLAN_SIMULATION=1','-DPIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION=1',
  '-DPIONEER_Q38_HEAD_MATRIX_SIMULATION=1','-DPIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION=1',
  '-DPIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT=1']
 subprocess.run([*common,*simulation,str(here/'pioneer_q38_hybrid_head_root_helpers_test.cpp'),'-o',str(helpers)],check=True)
 subprocess.run([str(helpers),str(probe),str(data['QGW8_STARTUP_HEAD_FIXTURE']),str(data['QGW8_STARTUP_HEAD_JOB'])],check=True,env=env)
print('PASS QGWC 7-bank retained startup and real linked ROOT accept the same QGWC head fixture; synthetic ledger, NO device')
