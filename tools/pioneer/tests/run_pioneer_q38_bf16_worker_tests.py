#!/usr/bin/env python3
"""Actual full BF16 worker/NoC-producer execution, synthetic byte endpoints."""
import json
import os
from pathlib import Path
import re
import subprocess
import sys
assert __debug__ and len(sys.argv)==4 and not os.environ.get('PYTHONOPTIMIZE')
old,image,out=map(Path,sys.argv[1:]);here=Path(__file__).resolve().parent
assert all(p.is_absolute() for p in (old,image,out)) and old.is_dir() and image.is_dir()
assert not out.exists() and not out.is_symlink()
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
out.mkdir(mode=0o700);exits={};commands=[]
common=[os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-ffp-contract=off','-fsanitize=address,undefined']
# The inherited generic ISA fixture retains its existing TEST switches.
# New BF16 shapes and posthoc arithmetic are compiled in separate HOST
# observer units WITHOUT these roles. No define/undef guard bypass.
flags=re.findall(r'-D[A-Z][A-Z0-9_]*(?:=[0-9]+)?',(here/'run_pioneer_q38_ffn_persistent_image_tests.sh').read_text())
def run(name,argv):
 commands.append([name,*map(str,argv)])
 with (out/(name+'.stdout')).open('xb') as stdout,(out/(name+'.stderr')).open('xb') as stderr:
  result=subprocess.run(list(map(str,argv)),stdout=stdout,stderr=stderr,check=False,
   env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
 exits[name]=result.returncode;(out/'actual-exits.json').write_text(json.dumps(exits,sort_keys=True,indent=2)+'\n')
 assert result.returncode==0 and not (out/(name+'.stderr')).stat().st_size,name
 print('CLOSED_BF16_WORKER_OFFBOARD',name,result.returncode,flush=True)
run('bridge-compile',[*common,'-c',here/'pioneer_q38_sigmoid_precision_host_bridge.cpp','-o',out/'bridge.o'])
run('shape-compile',[*common,'-c',here/'pioneer_q38_bf16_worker_probe.cpp','-o',out/'shape.o'])
run('worker-compile',[*common,*flags,here/'pioneer_q38_bf16_worker_test.cpp',out/'bridge.o',out/'shape.o','-o',out/'worker-test'])
run('worker',[out/'worker-test',old,image])
run('fraction',[sys.executable,here/'pioneer_q38_cached_bf16_fraction_test.py',out/'worker.stdout'])
(out/'test-argv.json').write_text(json.dumps(commands,indent=2)+'\n')
print('PASS_BF16_WORKER_PRODUCER_OFFBOARD children'+str(len(exits))+
 '_stderr0_NO_EXECUTED_Q4_DISPATCH_CONNECTED_ROOT_NATIVE_HARDWARE_MODEL_QUALITY_OR_TPS_AUTHORITY',flush=True)
