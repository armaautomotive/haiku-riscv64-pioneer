#!/usr/bin/env python3
import bisect
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys

elf=Path(sys.argv[1]);tool='/Volumes/HaikuBuildLocal/generated.riscv64/cross-tools-riscv64/bin/riscv64-unknown-haiku-objdump'
assert hashlib.sha256(elf.read_bytes()).hexdigest()=='62f24f28379cabacd81cce767f0d852a649da400551d8a31d9a42752a87d746b'
# Manually inspected compiler switch sites in this exact ELF. Each has a
# bounded (0..7 or 0..8) table index,an immutable table base and lw+jr.
# These are intra-function switch branches,not unresolved calls. Refuse
# a changed ELF and verify EVERY literal destination before using them.
switches={0x23028:(0x5e864,8),0x26048:(0x5e968,8),0x26f78:(0x5e9cc,9),
 0x4f7e8:(0x5e9f0,9),0x515bc:(0x5ea14,9),0x52620:(0x5ea38,9),
 0x55158:(0x5ea5c,9),0x5b2f0:(0x5ebfc,9),0x666b4:(0x462e0,9)}
raw={}
for row in subprocess.check_output([tool,'-s','-j','.text','-j','.conv','-j','.bf16',str(elf)],text=True).splitlines():
 p=row.split()
 if p and re.fullmatch('[0-9a-f]{5,8}',p[0]):
  a=int(p[0],16)
  for word in p[1:5]:
   if not re.fullmatch('[0-9a-f]{8}',word):break
   raw[a]=int.from_bytes(bytes.fromhex(word),'little');a+=4
functions={}
for row in subprocess.check_output([tool,'-t',str(elf)],text=True).splitlines():
 p=row.split()
 if len(p)==6 and p[2]=='F' and int(p[4],16):
  a,n=int(p[0],16),int(p[4],16)
  if a in functions:
   assert functions[a]['size']==n,(hex(a),p,functions[a]);functions[a]['name']+='|'+p[5]
  else:functions[a]={'name':p[5],'size':n,'frame':0,'edges':set()}
for a,name,n in ((0x6000,'_start',4),(0x40000,'scalar_entry',16),(0x60000,'scalar_fault_park',4),(0x60004,'q38_global_chain_stopped',4)):
 if a not in functions:functions[a]={'name':name,'size':n,'frame':0,'edges':set()}
starts=sorted(functions)
def owner(pc):
 at=bisect.bisect_right(starts,pc)-1;assert at>=0,hex(pc)
 a=starts[at];assert pc-a<functions[a]['size'],(hex(pc),functions[a]);return a
instructions={int(m[1],16):int(m[2],16) for m in re.finditer(r'^\s*([0-9a-f]+):\s+([0-9a-f]{8})\s',subprocess.check_output([tool,'-d',str(elf)],text=True),re.M)}
def signed(x,n):return x-(1<<n) if x>>(n-1) else x
def edge(f,target):
 other=owner(target)
 if other!=f:functions[f]['edges'].add(other)
unknown=[];calls=0;seenSwitches=set()
for f,info in functions.items():
 for pc in range(f,f+info['size'],4):
  assert pc in instructions,(info['name'],hex(pc));i=instructions[pc]
  op=i&127;rd=(i>>7)&31;s=(i>>15)&31;fun=(i>>12)&7
  if rd==2 and op in (0x13,0x33,0x03,0x37,0x17,0x67,0x6f):
   if op==0x13 and s==2 and fun==0:info['frame']+=max(0,-signed(i>>20,12))
   elif op==0x37 and pc==0x40000:assert i==0x00070137
   else:unknown.append(('stack',hex(pc),hex(i),info['name']))
  if op==0x67:
   imm=signed(i>>20,12)
   if rd==0 and s==1 and imm==0:continue
   prior=instructions.get(pc-4,0)
   if prior&127==0x17 and (prior>>7)&31==s:
    target=((pc-4)+signed(prior&0xfffff000,32)+imm)&0xfffffffe;edge(f,target);calls+=1
   elif pc in switches:
    assert rd==0 and imm==0 and instructions[pc-4]&127==3
    base,count=switches[pc]
    for index in range(count):assert owner(raw[base+index*4])==f,(hex(pc),hex(base),index)
    seenSwitches.add(pc)
   else:unknown.append(('indirect',hex(pc),hex(i),info['name']))
  elif op==0x6f:
   imm=((i>>31)<<20)|(((i>>21)&1023)<<1)|(((i>>20)&1)<<11)|(((i>>12)&255)<<12)
   edge(f,pc+signed(imm,21));calls+=rd!=0
  elif op==0x63:
   imm=((i>>31)<<12)|(((i>>25)&63)<<5)|(((i>>8)&15)<<1)|(((i>>7)&1)<<11)
   edge(f,pc+signed(imm,13))
assert not unknown,unknown
assert seenSwitches==switches.keys()
cache={}
def bound(a,visiting=()):
 assert a not in visiting,('cycle',[functions[x]['name'] for x in (*visiting,a)])
 if a in cache:return cache[a]
 best=max((bound(child,(*visiting,a)) for child in functions[a]['edges']),default=(0,[]),key=lambda p:p[0])
 result=(functions[a]['frame']+best[0],[a,*best[1]]);cache[a]=result;return result
total,path=bound(0x6000)
print(json.dumps({'conservativeBytes':total,'stackBytes':8192,'functions':len(functions),'calls':calls,
 'path':[{'address':hex(a),'name':functions[a]['name'],'frame':functions[a]['frame']} for a in path],
 'scope':'sum of all negative SP immediates per ELF function plus all direct call/tail/branch callees;no execution or model proof'},indent=2))
assert total<=8192,('conservative whole-ROOT stack exceeds reservation',total)
