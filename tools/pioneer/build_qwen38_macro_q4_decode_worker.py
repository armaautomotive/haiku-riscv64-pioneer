#!/usr/bin/env python3
"""Build a complete macro-Q4 shader from an authenticated vector-Q4 control.

No native package,driver,installation,card access or speed claim is made.
The supplied control must first reproduce its entire old matrix object and
whole ELF from current sources/the SAME link objects. Changed prerequisite
objects therefore fail before the new candidate can be linked.
"""
import ast
import hashlib
import os
import pathlib
import re
import subprocess
import sys
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parent/'tests'))
from pioneer_q38_vector_q4_relocations import audit

PINS=(
 ('boot.bin',4,'a12af9d63593a1f882750a147dd7a511ac476c5c7a55526bd4d51c685173991c'),
 ('globalworker.bin',9232,'e4dd94af8e3f828bff6482fa255266356de46b28bf5a419816d76e32de98f2b3'),
 ('precise.bin',8392,'32c078029e1c74ec4bd9744ea494433f5003f63a936df05445d3066c722a7ecc'),
 ('dense.bin',10332,'d1749707ffe803e28dced9c7d065e763f125009678ace33b4d468109984869ad'),
 ('fetch.bin',1292,'22b6fa8a631dd9b952adb8d624888bce309bf24392f004eecd93f9becf991df8'),
 ('text.bin',8148,'a69573017671d2bb86798b392fd38e13b5f8d1a4ac37a9aa7daaeddfc15757cd'),
 ('affine.bin',4880,'eb4b1bc4bc8d766b83083b3cda183fd9d8ad97c9e5eee6d27d93da451dc45b06'),
 ('ffn.bin',16456,'051499d768f11fc58757529ae1e639a0cda66c584378021b1b4a03dab5c44c2c'),
 ('service.bin',9888,'6e0219d9e66e25df248b1547f0fb642138434e13a5d043bfe3b815e773ec8f3a'),
 ('gdn.bin',35944,'e699263135d4815d9fcf9d41cfdca164d1935862949239f83f89cd4d5e78c77a'),
 ('program.elf',145148,'89f71fc9d6e8006d5267ed566c7a288f2406787fae10f91752c2874187c1c5f3'),
) # Independently reproduced shader bytes,NOT native or hardware authority.
args=sys.argv[1:];inspect='--inspect-candidate' in args
if inspect:
 assert args.count('--inspect-candidate')==1;args.remove('--inspect-candidate')
assert len(args)==3
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
sdk,control,out=map(pathlib.Path,args)
here=pathlib.Path(__file__).resolve().parent
assert all(p.is_absolute() for p in (sdk,control,out)) and control.is_dir()
assert not control.is_symlink() and not out.exists() and not out.is_symlink()
pins=next(ast.literal_eval(n.value) for n in ast.parse((here/'build_qwen38_vector_q4_metadata_worker.py').read_text()).body
          if isinstance(n,ast.Assign) and any(isinstance(t,ast.Name) and t.id=='PINS' for t in n.targets))
assert pins
for name,size,digest in pins:
 p=control/name;raw=p.read_bytes()
 assert not p.is_symlink() and len(raw)==size and hashlib.sha256(raw).hexdigest()==digest,name
out.mkdir(mode=0o700)
tool=str(sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku-')
command=(control/'compile-math-argv.txt').read_text().splitlines()
assert command[0]==tool+'g++' and command.count('-c')==1 and command.count('-o')==1
oldsource=pathlib.Path(command[command.index('-c')+1])
assert oldsource.name=='blackhole_qwen38_vector_q4_metadata_math.cpp' and oldsource.is_absolute()
oldhere=oldsource.parent
command=[str(here/pathlib.Path(p).relative_to(oldhere)) if p.startswith(str(oldhere)+'/') else p for p in command]
original=pathlib.Path(command[command.index('-o')+1])
assert original.parent==control
def run(*values):subprocess.run(list(map(str,values)),check=True)
command[command.index('-o')+1]=str(out/'unchanged-vector.o')
run(*command)
assert (out/'unchanged-vector.o').read_bytes()==original.read_bytes(),'ENTIRE accepted matrix object FIRST'
link=(control/'link-argv.txt').read_text().splitlines()
assert link[0]==tool+'ld' and link.count(str(original))==1
oldlink=list(link);oldlink[oldlink.index('-o')+1]=str(out/'unchanged.elf')
oldlink[oldlink.index('-Map')+1]=str(out/'unchanged.map')
run(*oldlink)
assert (out/'unchanged.elf').read_bytes()==(control/'program.elf').read_bytes(),'ENTIRE old link/object closure FIRST'
command[command.index('-c')+1]=str(here/'blackhole_qwen38_macro_q4_decode_math.cpp')
command[command.index('-o')+1]=str(out/'macro-program.o')
command=[command[0],'-DPIONEER_Q38_MACRO_Q4_DECODE_IMAGE=1','-DPIONEER_Q38_MACRO_Q4_DECODE=1',*command[1:]]
run(*command)
(out/'compile-math-argv.txt').write_text('\n'.join(command)+'\n')
script=(control/'program.ld').read_text();anchor='\t.affine 0x70b00 : {'
assert script.count(anchor)==1
script=script.replace(anchor,anchor+'\n\t\t*(.text._ZN23PioneerQ38MacroQ4Decode*)'
 '\n\t\t*(.text._Z17PioneerSfpuLoad32I18VectorPanelEmitterE*)'
 '\n\t\t*(.text._Z22PioneerQ38VectorAffineI18VectorPanelEmitterE*)')
(out/'program.ld').write_text(script)
link=[str(out/'macro-program.o') if p==str(original) else p for p in link]
for option,name in (('-T','program.ld'),('-Map','program.map'),('-o','program.elf')):
 link[link.index(option)+1]=str(out/name)
run(*link);(out/'link-argv.txt').write_text('\n'.join(link)+'\n')
elf=out/'program.elf'
assert not subprocess.check_output([tool+'nm','-u',str(elf)]).strip()
symbols=subprocess.check_output([tool+'nm','-n',str(elf)],text=True)
names=subprocess.check_output([tool+'nm','-nC',str(elf)],text=True)
assert not re.search(r'^[0-9a-f]+ [bBdDgGsS] ',symbols,re.M)
for name in ('PioneerQ38MacroQ4Decode::Setup','PioneerQ38MacroQ4Decode::Decode','PioneerQ38MacroQ4Decode::Finish',
             'PioneerQ38VectorQ4Metadata::Block','PioneerQ38WideReturningEmitter','q38_compute_once'):
 assert name in names,name
dis=subprocess.check_output([tool+'objdump','-d',str(elf)],text=True)
assert not re.search(r'\b(?:divu?|remu?|mulh[su]*|fdiv\.[sd]|fsqrt\.[sd]|fmadd|fmsub|fnmadd|fnmsub|fcvt\.wu?\.s|ebreak)\b',dis)
reserved=[row for row in dis.splitlines() if re.search(r'\bs11\b',row)]
assert len(reserved)==2 and sum(bool(re.search(r'\bmv\s+s11,\w+\s*$',row)) for row in reserved)==1
assert sum(bool(re.search(r'\bmv\s+\w+,s11\s*$',row)) for row in reserved)==1
frames=[r.split('\t') for r in (out/'macro-program.su').read_text().splitlines()]
assert frames and all(len(r)==3 and r[2]=='static' and int(r[1])<=4096 for r in frames)
sections=[name[:-4] for name,_,_ in pins if name.endswith('.bin')]
rows=subprocess.check_output([tool+'objdump','-h',str(elf)],text=True).splitlines()
for i,row in enumerate(rows):
 m=re.match(r'\s*\d+\s+(\S+)\s+([0-9a-fA-F]+)\s+',row)
 if m and 'ALLOC' in rows[i+1]:assert m[1][1:] in sections,row
for section in sections:run(tool+'objcopy','-O','binary','-j','.'+section,elf,out/(section+'.bin'))
for section,limit in (('fetch',0x700),('affine',0x1500),('text',0x2000),('service',0x6000)):
 assert 0<(out/(section+'.bin')).stat().st_size<=limit
audit(control,out,tool)
(out/'program.symbols').write_text(symbols);(out/'program.disassembly').write_text(dis)
run(sys.executable,here/'tests/pioneer_q38_gdn_rendezvous_csr_gate.py',out/'program.disassembly')
observed=[]
for name,_,_ in pins:
 raw=(out/name).read_bytes();item=(name,len(raw),hashlib.sha256(raw).hexdigest())
 observed.append(item);print(item,flush=True)
if not inspect:assert tuple(observed)==PINS,'whole shader candidate identity changed'
print('BUILD_ONLY complete macro-Q4 worker/original vector metadata and original weights/old closure reproduced/'
      'NO native ownership,physical use,quality,TPS or speedup grant',flush=True)
