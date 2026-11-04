#!/usr/bin/env python3
"""Actual frozen shader macro-command scope;no hardware IO or speed claim."""
import os
import pathlib
import struct
import subprocess
import sys
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parent.parent))
from generate_qwen38_token_programs import authenticated,pins

assert len(sys.argv)==2 and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
here=pathlib.Path(__file__).resolve().parent;directory=pathlib.Path(sys.argv[1])
raw=authenticated(directory,pins(here.parent/'build_qwen38_macro_q4_decode_worker.py','PINS'))
tool='/Volumes/HaikuBuildLocal/generated.riscv64/cross-tools-riscv64/bin/riscv64-unknown-haiku-'
maps={'boot':0x6000,'globalworker':0x20000,'precise':0x30000,'dense':0x40000,'ffn':0x60000,
      'service':0x68000,'fetch':0x70000,'affine':0x70b00,'text':0x72000,'gdn':0xb8000}
functions=[]
for row in subprocess.check_output([tool+'readelf','-sW',str(directory/'program.elf')],text=True).splitlines():
 p=row.split()
 if len(p)==8 and p[3]=='FUNC' and int(p[2]):functions.append((int(p[1],16),int(p[2]),p[7]))
assert functions
observed={}
for at,size,name in functions:
 spans=[(base,raw[section+'.bin']) for section,base in maps.items()
        if base<=at and at+size<=base+len(raw[section+'.bin'])]
 assert len(spans)==1 and size%4==0,(hex(at),size,name)
 base,data=spans[0]
 for offset in range(0,size,4):
  word=struct.unpack_from('<I',data,at-base+offset)[0]
  if word&3==3:continue # ordinary RV32 instruction,not inline Tensix
  op=((word>>2)|(word<<30))&0xffffffff
  if op>>24 not in (0x91,0x93):continue
  pc=at+offset
  assert 'PioneerQ38MacroQ4Decode' in name,('macro configuration escapes private job helper',hex(pc),name,hex(op))
  kind=next(k for k in ('5Setup','6Decode','6Finish') if k in name)
  assert pc not in observed or observed[pc]==(kind,op)
  observed[pc]=(kind,op)
assert len(observed)==28
def audit(values):
 expected={'5Setup':[0x91000000,0x91000010,0x91000441,0x91000551,0x91000061,0x91000071,0x91000081],
           '6Finish':[0x91000041,0x91000051,0x91000061,0x91000071,0x91000081],
           '6Decode':[]}
 for upper in (True,False):
  for block in (0,4,8,12):
   expected['6Decode'] += [0x930d0000|(int(upper)<<22)|block,0x931d0000|(int(upper)<<22)|block|2]
 for kind,words in expected.items():assert [v for _,(k,v) in sorted(values.items()) if k==kind]==words,kind
audit(observed)
negative=0
for key in sorted(observed):
 for replacement in (0x910000f1,0x93fd0001): # LaneConfig write or invalid macro/register/address
  changed=observed.copy();kind,_=changed[key];changed[key]=(kind,replacement)
  try:audit(changed)
  except AssertionError:negative+=1
  else:raise AssertionError('mutated macro command accepted')
print('PASS_FROZEN_MACRO_Q4_ROM_SCOPE all10sections/actualFUNCbytes/config12ONLY_SetupFinish/load16ONLY_Decode/'
      'noLaneConfig,noMacroStore,noPacker;mutantRefusals'+str(negative)+';'
      'COMBINE_WITH_REAL_JOB_RETURN_AND_EXCLUSIVE_NATIVE_OWNER_TESTS_NOT_HARDWARE_OR_TPS',flush=True)
