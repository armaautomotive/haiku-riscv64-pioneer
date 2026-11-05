#!/usr/bin/env python3
"""Whole E -> fixed-config image boundary; new bodies are BOUND,not proved.

Every other byte is unchanged except ONE resolved call to the COMPLETE
unchanged GroupWindow helper. No native,ownership,engine or speed grant.
"""
import functools,hashlib,pathlib,struct
from pioneer_q38_queued_q4_block_relocations import load_image,require,WORK,BFP,WINDOW,SUBJECTS as OLD_SUBJECTS
from pioneer_q38_vector_q4_relocations import BASES
TWO_SETUP='_ZN30PioneerQ38ExactQ4TwoProductDot5SetupI28PioneerQ38ExactQ4WorkEmitterEEbRT_'
GROUP='_ZL15PreparedQ4GroupPVKjjRjS1_'
COMMON={
 '_ZL7Counterj':(0x6a6a0,56),
 '_ZL7IdleAllv':(0x6a6d8,140),
 '_ZL5Routej':(0x6a764,120),
 '_ZL14PreparedQ4ReadjjjjRjS_':(0x6a7dc,164),
 '_ZL13SourceMatchesRA16_Kj':(0x6a880,136),
 '_ZL16PreparedQ4Inputsj':(0x6a908,516),
 GROUP:(0x6ab0c,388),
 WINDOW:(0x6c288,488),
}
SUBJECTS={
 WORK:(0x6ac90,5424,'e8630c37b8ba839b48e38a9918ac9a01d5b739fd11645c385e541aa3c9c90f58'),
 TWO_SETUP:(0x6c1c0,544,'0dd5d62a73e555bdc27f9d5f5ff026a7f3a7bf5661cca2eab944b642efc1b3dd'),
 BFP:(0x6c5c8,1796,'57a6f3c64e9b3e932d4b4138f12dcadff40bdc83e7b57f58be2f4b2fd0a7474b'),
}
@functools.lru_cache(maxsize=1)
def records():
 import ast
 path=pathlib.Path(__file__).resolve().parent.parent/'build_qwen38_queued_q4_block_worker.py'
 return next(ast.literal_eval(n.value) for n in ast.parse(path.read_text()).body if isinstance(n,ast.Assign) and any(isinstance(t,ast.Name) and t.id=='INSPECT_PINS' for t in n.targets))
def audit_data(old,new,old_symbols,new_symbols):
 require(set(old)==set(new)==set(BASES),'whole ROM inventory')
 for name,size,digest in records():
  if name=='program.elf':continue
  data=old[name.removesuffix('.bin')]
  require(len(data)==size and hashlib.sha256(data).hexdigest()==digest,'whole authenticated E control '+name)
 require(len(new['service'])==19660,'fixed service extent')
 for section in BASES:
  if section!='service':require(old[section]==new[section],'whole unchanged '+section)
 require(old['service'][:9888]==new['service'][:9888],'whole unchanged convolution prefix')
 maps=old_symbols,new_symbols;raws=old,new
 def contents(side,at,size):
  require(0x68000<=at and at+size<=0x68000+len(raws[side]['service']),'service body bounds')
  return raws[side]['service'][at-0x68000:at-0x68000+size]
 for side in (0,1):
  expected=dict(COMMON)
  if side:expected[WINDOW]=(0x6c3e0,488)
  subjects=SUBJECTS if side else OLD_SUBJECTS
  expected.update({name:(at,size) for name,(at,size,_) in subjects.items()})
  limit=0x68000+len(raws[side]['service'])
  tail=sorted((at,*item) for at,item in maps[side].items() if 0x6a6a0<=at<limit)
  require(len(tail)==len(expected),'no extra or missing tail symbols')
  require({name:(at,size) for at,name,size,kind in tail}==expected and all(kind=='FUNC' for _,_,_,kind in tail),'whole exact tail symbol inventory')
  cursor=0x6a6a0
  for at,name,size,kind in tail:
   require(cursor<=at and at-cursor<=12 and size>0 and at+size<=limit,'nonoverlapping full function '+name)
   require(contents(side,cursor,at-cursor)==bytes(at-cursor),'zero alignment only')
   cursor=at+size
  require(cursor==limit,'no trailing unexamined code')
  for name,(at,size,digest) in subjects.items():
   require(hashlib.sha256(contents(side,at,size)).hexdigest()==digest,'whole BOUND subject '+name)
 # One AUIPC/JALR pair,all opcode/register bits unchanged;target is the
 # complete unchanged Window function in the actual ELF,not an offset guess.
 pairs=[]
 for side in (0,1):
  a,b=struct.unpack('<II',contents(side,0x6abc4,8))
  require(a&0xfff==0x097 and b&0xfffff==0x080e7,'exact call opcode/register pair')
  high=(a&0xfffff000)-(2**32 if a>>31 else 0)
  low=(b>>20)-(4096 if b>>31 else 0)
  target=(0x6abc4+high+low)&0xffffffff
  require(maps[side].get(target)==(WINDOW,488,'FUNC'),'actual complete relocation target')
  pairs.append((a,b,target))
 require(pairs[1][2]-pairs[0][2]==344,'exact bounded relocation delta')
 normalized=bytearray(new['service'])
 normalized[0x2bc4:0x2bcc]=old['service'][0x2bc4:0x2bcc]
 for name,(at,size) in COMMON.items():
  new_at=0x6c3e0 if name==WINDOW else at
  require(contents(0,at,size)==bytes(normalized[new_at-0x68000:new_at-0x68000+size]),'whole unchanged helper '+name)
 return pairs
def audit(control,image,tool):
 old,old_symbols=load_image(control,tool);new,new_symbols=load_image(image,tool)
 result=audit_data(old,new,old_symbols,new_symbols)
 print('BOUND_FIXED_CONFIG_WHOLE_IMAGE nineUnchangedSections complete9888BPrefix eightCompleteHelpers oneResolvedCall threeBoundNEWBodies NOT_ARITHMETIC_OWNERSHIP_HARDWARE_OR_SPEED_PROOF',flush=True)
 return result
