#!/usr/bin/env python3
"""Authenticate and test the tag100 ROOT-spill firmware DATA/address map."""
import hashlib
import os
import pathlib
import re
import subprocess
import sys
import tempfile
from compare_pioneer_q38_observed_trace import require
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parent.parent))
from generate_qwen38_token_programs import authenticated
from PioneerQ38SpillQ4ArtifactPins import ROOT_PINS,WORKER_PINS,SFPNOP_WORKER_PINS

args=sys.argv[1:];sfpnop='--sfpnop' in args
if sfpnop:
 require(args.count('--sfpnop')==1,'single SFPNOP variant');args.remove('--sfpnop')
require(len(args)==2 and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ),'arguments/environment')
root,worker=map(pathlib.Path,args);here=pathlib.Path(__file__).resolve().parent;pioneer=here.parent
root_data=authenticated(root,ROOT_PINS);worker_pins=SFPNOP_WORKER_PINS if sfpnop else WORKER_PINS
worker_data=authenticated(worker,worker_pins)
header=pioneer/('PioneerQ38FullOffloadSpillQ4SfpNopPrograms.h' if sfpnop else 'PioneerQ38FullOffloadSpillQ4Programs.h')
expected='dadcb5038bc3b783bd276b5322160499d49f69f68b371c98ec9cff00dfe5c18b' if sfpnop else 'b53bf562c507fde28a7cade431d23a25d913742f0a3da055a2ae19d1598d8ccb'
require(hashlib.sha256(header.read_bytes()).hexdigest()==expected,'exact generated tag100 DATA header')
role=pioneer.parent.parent/'src/add-ons/kernel/drivers/misc/PioneerQ38FullOffloadRole.h'
required=set(re.findall(r'!defined\(([A-Z_][A-Z0-9_]*)\)\s*\|\|\s*\1\s*!=\s*1\b',role.read_text()))
require(len(required)==23,'full native role macro set')
base={name:'1' for name in required}
base.update(_KERNEL_MODE='1',PIONEER_DRIVER_BUILD_TAG='0x2026111500010101ULL' if sfpnop else '0x2026111500010100ULL',
 PIONEER_Q38_FULL_OFFLOAD_BATCHED='1',PIONEER_Q38_FULL_OFFLOAD_METRICS='1',
 PIONEER_Q38_FULL_OFFLOAD_MACRO_Q4='1',PIONEER_Q38_FULL_OFFLOAD_SPILL_Q4='1',B_DEVICE_OP_CODES_END='0x1000')
if sfpnop:base['PIONEER_Q38_FULL_OFFLOAD_SPILL_Q4_SFPNOP']='1'
definitions=''.join('#define '+name+' '+value+'\n' for name,value in sorted(base.items()))
with tempfile.TemporaryDirectory(prefix='pioneer-spill-q4-data-',dir='/private/tmp') as temporary:
 out=pathlib.Path(temporary);(out/'full_offload_spill_q4_role_defines.inc').write_text(definitions)
 for directory,files in ((root,ROOT_PINS),(worker,worker_pins)):
  target=out/('root' if directory==root else 'worker');target.mkdir(mode=0o700)
  for name,_,_ in files:(target/name).write_bytes((root_data if directory==root else worker_data)[name])
 binary=out/'data';command=[os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror',
  '-fsanitize=address,undefined','-I'+str(out),'-I'+str(pioneer),str(here/'pioneer_q38_full_offload_spill_q4_data_test.cpp'),'-o',str(binary)]
 subprocess.run(command,check=True)
 subprocess.run([str(binary),str(out/'root'),str(out/'worker')],check=True,
  env={**os.environ,'UBSAN_OPTIONS':'halt_on_error=1','ASAN_OPTIONS':'quarantine_size_mb=32'})
print('PASS '+('tag101 SFPNOP' if sfpnop else 'tag100')+' full-card Q4 DATA:five ROOT spans incl. journaled spill, all ten Q4 peers, exact every code word/address, STOP gaps and 120-worker bounds;NO native install, inference or TPS',flush=True)
