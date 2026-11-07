#!/usr/bin/env python3
"""QGW7 queued-counter ROOT inspection. No native or device operations."""
import hashlib
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile

assert __debug__ and len(sys.argv)==4 and not os.environ.get('PYTHONOPTIMIZE')
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
sdk,first,second=map(Path,sys.argv[1:]);assert all(p.is_absolute() and p.is_dir() for p in (sdk,first,second))
here=Path(__file__).resolve().parent;root=here.parent
pins=(('boot.bin',4,'32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca'),
 ('text.bin',130296,'6014d392b8103a5ac0d9bf5c15704dc75ebe0c0a43a22d58f041c05cc534ca9b'),
 ('terminal.bin',8,'6eb1d84addfac7202ad4f0d747c6e64d3b92c03025651c7dabe6c56c9cba95d5'),
 ('conv.bin',21328,'2b3baac417c2c672b516e863409b166b34413d0e5952798e24c17912c2916705'),
 ('bf16.bin',34344,'6ee1c1ac9cae9059933eb1938fabd6e9b471bdc67aab2497048d1f023c6ae756'),
 ('program.elf',217268,'567e04f17e465deb634d51d2469f732d40995acc8343399148b2f37629a56f5b'))
for name,size,digest in pins:
 for folder in (first,second):
  data=(folder/name).read_bytes();assert len(data)==size and hashlib.sha256(data).hexdigest()==digest,(folder,name)
tool=str(sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku-')
symbols=subprocess.check_output([tool+'nm','-nSC',str(first/'program.elf')],text=True)
rows=[]
for line in symbols.splitlines():
 parts=line.split(maxsplit=3)
 if len(parts)==4:
  try:rows.append((int(parts[0],16),int(parts[1],16),parts[3]))
  except ValueError:pass
def matching(name):return [(at,size) for at,size,label in rows if label.startswith(name+'(')]
gather=matching('PioneerQ38CounterGather::Read');wrapper=matching('DensePartitionRoot::GatherCounters')
assert len(gather)==len(wrapper)==1 and 0x20000<=gather[0][0]<0x30000
dis=subprocess.check_output([tool+'objdump','-d','-C',str(first/'program.elf')],text=True)
calls=[]
for line in dis.splitlines():
 m=re.match(r'\s*([0-9a-f]+):.*\bjalr\b.*#\s*([0-9a-f]+)\s',line)
 if m and int(m[2],16)==wrapper[0][0]:calls.append(int(m[1],16))
for label in ('Projection::BfGatherProducer','Projection::BfGatherMatrix'):
 functions=matching(label);assert len(functions)==2,(label,functions)
 assert sum(any(start<=call<start+size for call in calls) for start,size in functions)==1,(label,calls)
assert len(calls)==2,calls
parents=['PIONEER_Q38_HYBRID_QUEUED_COUNTERS_IMAGE','PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE',
 'PIONEER_Q38_LAYER_COMPONENT_RECURRENT','PIONEER_Q38_GLOBAL_CHAIN_BOUNDARY_SEALS',
 'PIONEER_Q38_GLOBAL_CHAIN_HYBRID_BFP','PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION',
 'PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT']
flags=['-D'+name+'=1' for name in parents];role=root/'PioneerQ38HybridQueuedCountersRole.h'
def check(selected,valid):
 result=subprocess.run([tool+'g++','-std=c++17',*selected,'-fsyntax-only','-x','c++',str(role)],capture_output=True)
 assert (result.returncode==0)==valid,result.stderr.decode()[:1000]
check(flags,True)
for name in parents:
 remaining=[v for v in flags if not v.startswith('-D'+name+'=')]
 # Removing the profile itself is a valid ordinary build; every other
 # required parent must refuse both absence and defined-zero.
 if name!='PIONEER_Q38_HYBRID_QUEUED_COUNTERS_IMAGE':check(remaining,False)
 check(remaining+['-D'+name+'=0'],False)
for forbidden in ('PIONEER_Q38_COUNTER_GATHER_IMAGE','PIONEER_Q38_TID_BOUNDARY_IMAGE','_KERNEL_MODE',
 'PIONEER_DRIVER_BUILD_TAG','PIONEER_Q38_TOKEN_NATIVE_CONTEXT','PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE',
 'PIONEER_Q38_LAYER_COMPONENT_DENSE','PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE',
 'PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_FULL_MODEL_CPU_REFERENCE','__FAST_MATH__'):
 check(flags+['-D'+forbidden+'=1'],False)
with tempfile.TemporaryDirectory(prefix='pioneer-qgw7-queued-counters-',dir='/private/tmp') as directory:
 tmp=Path(directory);dummy=tmp/'dummy';entries=tmp/'entries';binary=tmp/'test'
 dummy.write_text('40000 40000 40000\n');entries.write_text(f'{gather[0][0]:x} 60000\n')
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror',
  '-fsanitize=address,undefined',str(here/'pioneer_q38_hybrid_queued_counters_test.cpp'),'-o',str(binary)],check=True)
 subprocess.run([str(binary),str(first),str(dummy),str(entries)],check=True,
  env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
print('PASS independently reproduced QGW7 queued-counter ROOT, exact emitted BF callsites and isolated roles; NO hardware or speed')
