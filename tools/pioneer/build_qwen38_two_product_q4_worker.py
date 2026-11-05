#!/usr/bin/env python3
"""Distinct OFFBOARD two-product worker;NO native/ROOT/physical selector."""
import hashlib
import os
import pathlib
import re
import runpy
import subprocess
import sys

here=pathlib.Path(__file__).resolve().parent
sys.path.insert(0,str(here/'tests'))
from pioneer_q38_prepared_q4_relocations import audit

PINS=(
 ('boot.bin',4,'a12af9d63593a1f882750a147dd7a511ac476c5c7a55526bd4d51c685173991c'),
 ('globalworker.bin',9504,'35bb8e77470d792b6a040b4aed99b2c966f2a7fb8e1a16a16deb3f9fc4bd5496'),
 ('precise.bin',8392,'32c078029e1c74ec4bd9744ea494433f5003f63a936df05445d3066c722a7ecc'),
 ('dense.bin',10332,'d1749707ffe803e28dced9c7d065e763f125009678ace33b4d468109984869ad'),
 ('fetch.bin',1292,'a74f8f6303787360f35b588e22746074adc9bcc780ae5a274be25331f765f4c6'),
 ('text.bin',7844,'fa783006d1943e886d777982f4aecc93fc0c321937e15f6d87755a14ddc07e00'),
 ('affine.bin',4680,'a8f650e58052aba96187bcbee08c25a24de4b8aa8d92c5bb7c34ccfeeca4682d'),
 ('ffn.bin',16456,'e6533bc2035a251dbc0f4aad1da9d5222fcf8649ff2577db375843dc746014b7'),
 ('service.bin',17636,'184c0d1c7b039706a246425c627910ddafb57482ad6682eb4b35280f4bdedd48'),
 ('gdn.bin',35944,'7ae46e0cea089f93ae7662d2033e469393c4a39859dbc3745d40c456033944c3'),
 ('program.elf',154332,'45707f586c7575a36b066bfe2c5addc7909fe64bf297de299b2df03fb345466f'),
) # Main + independent clean BUILD identity;not native/physical admission.
if not __debug__ or os.environ.get('PYTHONOPTIMIZE'):raise SystemExit('Refuse optimized Python')
args=sys.argv[1:];inspect='--inspect-candidate' in args
if inspect:assert args.count('--inspect-candidate')==1;args.remove('--inspect-candidate')
assert len(args)==2 and (inspect or PINS),'SDK OUTPUT --inspect-candidate until independently pinned'
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
sdk,out=map(pathlib.Path,args)
assert sdk.is_absolute() and sdk.is_dir() and out.is_absolute() and not out.exists() and not out.is_symlink()
out.mkdir(mode=0o700);saved=sys.argv[:]
try:
 sys.argv=[str(here/'build_qwen38_prepared_exact_q4_worker.py'),str(sdk),str(out/'unchanged-prepared')]
 old=runpy.run_path(sys.argv[0],run_name='__main__')
finally:sys.argv=saved
control=out/'unchanged-prepared';tool=old['tool']
assert hashlib.sha256((control/'program.elf').read_bytes()).hexdigest()=='b283d788a682681f8994058ab51b19b2b1e79f72945d7e839c9cdd81737ff9c3'
command=(control/'compile-math-argv.txt').read_text().splitlines()
original=command[command.index('-o')+1]
command[command.index('-o')+1]=str(out/'unchanged-math.o')
subprocess.run(command,check=True)
assert (out/'unchanged-math.o').read_bytes()==pathlib.Path(original).read_bytes(),'ENTIRE old math object FIRST'
command[command.index('-o')+1]=str(out/'math.o')
command=[command[0],'-DPIONEER_Q38_PREPARED_TWO_PRODUCT_IMAGE=1',*command[1:]]
subprocess.run(command,check=True)
(out/'compile-math-argv.txt').write_text('\n'.join(command)+'\n')
layout=(control/'program.ld').read_text();anchor='*(.text.pioneer_q38_resident_service)'
assert layout.count(anchor)==1
layout=layout.replace(anchor,anchor+'\n\t\t*(.text.*PioneerQ38ExactQ4TwoProduct*)')
(out/'program.ld').write_text(layout)
link=[str(out/'math.o') if value==original else value for value in old['link']]
assert sum(value==str(out/'math.o') for value in link)==1
for option,name in (('-T','program.ld'),('-Map','program.map'),('-o','program.elf')):link[link.index(option)+1]=str(out/name)
subprocess.run(link,check=True);(out/'link-argv.txt').write_text('\n'.join(link)+'\n')
elf=out/'program.elf'
assert not subprocess.check_output([tool+'nm','-u',str(elf)]).strip()
symbols=subprocess.check_output([tool+'nm','-n',str(elf)],text=True)
assert not re.search(r'^[0-9a-f]+ [bBdDgGsS] ',symbols,re.M),'no writable globals/GOT'
dis=subprocess.check_output([tool+'objdump','-d',str(elf)],text=True)
assert not re.search(r'\b(?:divu?|remu?|mulh[su]*|fdiv\.[sd]|fsqrt\.[sd]|fmadd|fmsub|fnmadd|fnmsub|fcvt\.wu?\.s|ebreak)\b',dis)
reserved=[row for row in dis.splitlines() if re.search(r'\bs11\b',row)]
assert len(reserved)==2 and sum(bool(re.search(r'\bmv\s+s11,\w+\s*$',row)) for row in reserved)==1
assert sum(bool(re.search(r'\bmv\s+\w+,s11\s*$',row)) for row in reserved)==1
frames=[row.split('\t') for row in (out/'math.su').read_text().splitlines()]
assert frames and all(len(row)==3 and row[2]=='static' and int(row[1])<=4096 for row in frames)
sections=old['sections'];rows=subprocess.check_output([tool+'objdump','-h',str(elf)],text=True).splitlines()
for i,row in enumerate(rows):
 m=re.match(r'\s*\d+\s+(\S+)\s+([0-9a-fA-F]+)\s+',row)
 if m and 'ALLOC' in rows[i+1]:assert m[1][1:] in sections,row
for section in sections:
 subprocess.run([tool+'objcopy','-O','binary','-j','.'+section,str(elf),str(out/(section+'.bin'))],check=True)
for section,limit in (('globalworker',0x10000),('fetch',0x700),('affine',0x1500),('text',0x2000),('service',0x6000)):
 assert 0<(out/(section+'.bin')).stat().st_size<=limit,(section,(out/(section+'.bin')).stat().st_size)
for section in ('boot','precise','dense','fetch','text','affine','ffn','gdn'):
 assert (out/(section+'.bin')).read_bytes()==(control/(section+'.bin')).read_bytes(),section
audit(control/'unchanged-original',out,tool,two_product=True)
(out/'program.symbols').write_text(symbols);(out/'program.disassembly').write_text(dis)
subprocess.run([sys.executable,str(here/'tests/pioneer_q38_gdn_rendezvous_csr_gate.py'),str(out/'program.disassembly')],check=True)
observed=[]
for name in (*(s+'.bin' for s in sections),'program.elf'):
 data=(out/name).read_bytes();assert data and not len(data)%4
 item=(name,len(data),hashlib.sha256(data).hexdigest());observed.append(item);print(item,flush=True)
if not inspect:assert tuple(observed)==PINS
print('BUILD_ONLY_TWO_PRODUCT_WORKER original_perK16_FP32 same_source_codec no_constant7_product;NO native/ROOT/physical/model/TPS grant',flush=True)
