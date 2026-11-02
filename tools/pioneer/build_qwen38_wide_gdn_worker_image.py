#!/usr/bin/env python3
"""Separate returning wide-GDN full worker BUILD;NEVER native/install/card IO."""
import hashlib
import os
import pathlib
import re
import runpy
import struct
import subprocess
import sys

PINS=(
 ('boot.bin',4,'a12af9d63593a1f882750a147dd7a511ac476c5c7a55526bd4d51c685173991c'),
 ('globalworker.bin',9232,'d820f6178c053c27e7380b774ab15733aff2a029d5e7d8de4e55f4549fbad955'),
 ('precise.bin',8392,'32c078029e1c74ec4bd9744ea494433f5003f63a936df05445d3066c722a7ecc'),
 ('dense.bin',10332,'d1749707ffe803e28dced9c7d065e763f125009678ace33b4d468109984869ad'),
 ('fetch.bin',1292,'8422f5a12befcf4bba1500919006a79c00accbe29ffa99c1750dff66dc871f77'),
 ('text.bin',6004,'d5fc264b4cb0a76ae84a68bd5762ff620b1b3eea38fd9bb3a34cbef4e76b4d8d'),
 ('affine.bin',5360,'0074182efb372d30e38807d26f701901c3995a5d3a549110b4ac35e5800ae51f'),
 ('ffn.bin',16456,'23ce6cc2dc07aa445b7babcac12a068dc3542001329cd24831e8e6749ff3a16f'),
 ('service.bin',9888,'7d5a42a087bc6bf9d24319583f3655ae344828a51b1092154f7b0567b0fdb187'),
 ('gdn.bin',35944,'c2824d4822c69867879b09fac67f9f9849cf5f7202c1beca668584976eaf4e9c'),
 ('program.elf',144608,'70fdf96d8edbc38d30391a5f180d5839e97581c9fb8e03f724c74be014e3e028'),
) # Independent main/clean BUILD identity ONLY,not execution/native authority.
args=sys.argv[1:];inspect='--inspect-candidate' in args
if inspect:
 assert args.count('--inspect-candidate')==1;args.remove('--inspect-candidate')
assert len(args)==2 and (inspect or PINS)
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
sdk,out=map(pathlib.Path,args);here=pathlib.Path(__file__).resolve().parent
assert sdk.is_absolute() and out.is_absolute() and not out.exists() and not out.is_symlink()
out.mkdir(mode=0o700);saved=sys.argv[:]
try:
 sys.argv=[str(here/'build_qwen38_token_twin_batch_prefetch_worker.py'),str(sdk),str(out/'unchanged-twin')]
 parent=runpy.run_path(sys.argv[0],run_name='__main__')
finally:sys.argv=saved
control=out/'unchanged-twin';tool=parent['tool']
assert hashlib.sha256((control/'program.elf').read_bytes()).hexdigest()=='56b470ae508655a070f17294c0a0fe242335214faed7101abaa05fb385ce6e38'
precise=parent['parent']['parent']['old'];mixed=precise['mixed']
unit=next(u for u in mixed['units'] if u[1]=='gdn_state')
assert unit[0]=='blackhole_qwen38_resident_gdn.cpp' and unit[3]=='-O2'
link=list(parent['link']);matches=[p for p in link if pathlib.Path(p).name=='gdn_state.o']
assert len(matches)==1;original=pathlib.Path(matches[0])
def run(*values):subprocess.run(list(map(str,values)),check=True)
command=[tool+'g++',*precise['flags'],unit[3],*mixed['fixed'],*unit[2],'-c',str(here/unit[0]),'-o',str(out/'unchanged-gdn_state.o')]
run(*command)
assert (out/'unchanged-gdn_state.o').read_bytes()==original.read_bytes(),'ENTIRE old GDN state object must reproduce FIRST'
obj=out/'wide_gdn_state.o';command[command.index('-c')+1]=str(here/'blackhole_qwen38_wide_returning_gdn.cpp')
command[command.index('-o')+1]=str(obj)
command=[command[0],'-DPIONEER_Q38_WIDE_GDN_RETURNING_IMAGE=1',*command[1:]]
run(*command);(out/'compile-gdn-argv.txt').write_text('\n'.join(command)+'\n')
(out/'program.ld').write_bytes((control/'program.ld').read_bytes())
link=[str(obj) if p==str(original) else p for p in link]
link[link.index('-T')+1]=str(out/'program.ld');link[link.index('-Map')+1]=str(out/'program.map');link[link.index('-o')+1]=str(out/'program.elf')
run(*link);(out/'link-argv.txt').write_text('\n'.join(link)+'\n')
elf=out/'program.elf';sections=parent['sections']
assert not subprocess.check_output([tool+'nm','-u',str(elf)]).strip()
symbols=subprocess.check_output([tool+'nm','-n',str(elf)],text=True)
names=subprocess.check_output([tool+'nm','-nC',str(elf)],text=True)
assert not re.search(r'^[0-9a-f]+ [bBdDgGsS] ',symbols,re.M)
for name in ('q38_local_recurrent_step','q38_resident_gdn_state_stage','q38_resident_persistent_gdn','q38_global_worker_main','PioneerQ38WideReturningEmitter'):
 assert name in names,name
assert 'PioneerQ38DeltaVectorHalf' not in names,'old narrow recurrence must not be linked'
dis=subprocess.check_output([tool+'objdump','-d',str(elf)],text=True)
assert not re.search(r'\b(?:divu?|remu?|mulh[su]*|fdiv\.[sd]|fsqrt\.[sd]|fmadd|fmsub|fnmadd|fnmsub|fcvt\.wu?\.s|ebreak)\b',dis)
reserved=[row for row in dis.splitlines() if re.search(r'\bs11\b',row)]
assert len(reserved)==2 and sum(bool(re.search(r'\bmv\s+s11,\w+\s*$',r)) for r in reserved)==1
assert sum(bool(re.search(r'\bmv\s+\w+,s11\s*$',r)) for r in reserved)==1
frames=[r.split('\t') for r in obj.with_suffix('.su').read_text().splitlines()]
assert frames and all(len(r)==3 and r[2]=='static' and int(r[1])<=2048 for r in frames)
section_rows=subprocess.check_output([tool+'objdump','-h',str(elf)],text=True).splitlines();bases={}
for i,row in enumerate(section_rows):
 m=re.match(r'\s*\d+\s+(\S+)\s+([0-9a-fA-F]+)\s+([0-9a-fA-F]+)',row)
 if m and 'ALLOC' in section_rows[i+1]:
  assert m[1][1:] in sections,row;bases[m[1][1:]]=int(m[3],16)
for section in sections:run(tool+'objcopy','-O','binary','-j','.'+section,elf,out/(section+'.bin'))
assert 0<len((out/'gdn.bin').read_bytes())<=0x10000
def sized_symbols(path):
 result={}
 for row in subprocess.check_output([tool+'nm','-S','--defined-only',str(path)],text=True).splitlines():
  p=row.split()
  if len(p)==4 and p[2] in ('t','T','w','W'):result[int(p[0],16)]=(p[3],int(p[1],16))
 return result
maps=(sized_symbols(control/'program.elf'),sized_symbols(elf))
gdn=((control/'gdn.bin').read_bytes(),(out/'gdn.bin').read_bytes())
relocations=0
for section in sections:
 if section=='gdn':continue
 raw=((control/(section+'.bin')).read_bytes(),(out/(section+'.bin')).read_bytes())
 assert len(raw[0])==len(raw[1]) and not len(raw[0])%4,section
 words=tuple(struct.unpack('<'+str(len(r)//4)+'I',r) for r in raw)
 changed={i for i in range(len(words[0])) if words[0][i]!=words[1][i]};covered=set()
 for i in range(len(words[0])-1):
  if not changed.intersection((i,i+1)):continue
  if not all(w[i]&0xfff==0x097 and w[i+1]&0xfffff==0x080e7 for w in words):continue
  targets=[]
  for side,w in enumerate(words):
   high=w[i]&0xfffff000;high-=(1<<32) if high>>31 else 0
   low=w[i+1]>>20;low-=4096 if low>>11 else 0
   at=bases[section]+i*4+high+low;name,size=maps[side][at]
   assert name in ('memcpy','memset') and size and at>=0xb8000,(section,hex(at),name)
   body=gdn[side][at-0xb8000:at-0xb8000+size];assert len(body)==size
   targets.append((name,size,body))
  assert targets[0]==targets[1],'relocated helper MUST retain its complete old instruction bytes'
  covered.update(changed.intersection((i,i+1)));relocations+=1
  print('AUDITED_SAME_HELPER_CALL_RELOCATION',section,hex(bases[section]+i*4),targets[0][0],targets[0][1],flush=True)
 assert changed==covered,(section,changed,covered)
(out/'program.symbols').write_text(symbols);(out/'program.disassembly').write_text(dis)
run(sys.executable,here/'tests/pioneer_q38_gdn_rendezvous_csr_gate.py',out/'program.disassembly')
observed=[]
for name in (*(s+'.bin' for s in sections),'program.elf'):
 data=(out/name).read_bytes();assert data and not len(data)%4
 item=(name,len(data),hashlib.sha256(data).hexdigest());observed.append(item);print(item,flush=True)
if not inspect:assert tuple(observed)==PINS,'new returning-wide worker is not independently frozen'
print('BUILD_ONLY new wide-GDN actual full twin worker/ENTIRE old56b+ancestors+GDN object FIRST/'+str(relocations)+' complete same-helper call relocations/NEW256B Delta scratch/13imports/NO native ownership,execution,numerical,physical,Work,TPS or speed grant',flush=True)
