#!/usr/bin/env python3
"""INSPECT ONLY unqualified complete queued-block worker; NEVER native/device."""
import hashlib,os,pathlib,re,subprocess,sys
from generate_qwen38_token_programs import authenticated,pins
INSPECT_PINS=(
 ('boot.bin',4,'a12af9d63593a1f882750a147dd7a511ac476c5c7a55526bd4d51c685173991c'),
 ('globalworker.bin',9720,'6b04a57faf2e6a17ee7ac9d9881420ed56a97b3e2b65bcbf0e7f215e093cb533'),
 ('precise.bin',8392,'32c078029e1c74ec4bd9744ea494433f5003f63a936df05445d3066c722a7ecc'),
 ('dense.bin',10332,'d1749707ffe803e28dced9c7d065e763f125009678ace33b4d468109984869ad'),
 ('ffn.bin',16456,'e6533bc2035a251dbc0f4aad1da9d5222fcf8649ff2577db375843dc746014b7'),
 ('service.bin',20416,'4f2a8204190644ac059db307d82594a87740e4b43fbec5e0d73d737d41764d0c'),
 ('fetch.bin',1292,'adc62da76663386222c2842cecb4cd02c65fe93f0a76bdd15a528d8510cb7ca7'),
 ('affine.bin',4680,'60b52b6031eaed56a710f1b42685bba596eac616cc10982a3328f23396a59fc6'),
 ('text.bin',7884,'85d7259c4ae19b8278bc234b11adbbed5871fbff6acd2f5c7f9c6f99a7c47dd2'),
 ('gdn.bin',35944,'7ae46e0cea089f93ae7662d2033e469393c4a39859dbc3745d40c456033944c3'),
 ('program.elf',155272,'8fee1277ca109e5e19423dbfdedeb746796c6eb1b46c1c06000c47c65c89ce8d'),
) # Immutable UNQUALIFIED prototype identity,NOT native/hardware permission.
if not __debug__ or os.environ.get('PYTHONOPTIMIZE'):raise SystemExit('Checks required')
assert len(sys.argv)==5 and sys.argv[-1]=='--inspect-candidate'
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
parent,replay,out=map(pathlib.Path,sys.argv[1:4])
assert all(p.is_absolute() and p.is_dir() and not p.is_symlink() for p in (parent,replay))
assert out.is_absolute() and not out.exists() and not out.is_symlink()
here=pathlib.Path(__file__).resolve().parent
baseline=authenticated(parent,pins(here/'build_qwen38_instruction_prefetch_worker.py','PINS'))
old_replay=authenticated(replay,pins(here/'build_qwen38_two_product_q4_worker.py','REPLAY_PINS'))
assert baseline['service.bin']==old_replay['service.bin']
out.mkdir(mode=0o700)
tool='/Volumes/HaikuBuildLocal/generated.riscv64/cross-tools-riscv64/bin/riscv64-unknown-haiku-'
command=(replay/'compile-math-argv.txt').read_text().splitlines()
assert command[0]==tool+'g++' and command[-4]=='-c' and command[-2]=='-o'
assert command[-3].endswith('/blackhole_qwen38_prepared_exact_q4_math.cpp')
assert command.count('-DPIONEER_Q38_PREPARED_TWO_PRODUCT_REPLAY_IMAGE=1')==1
old_object=command[-1];command[-3]=str(here/'blackhole_qwen38_prepared_exact_q4_math.cpp')
command[-1]=str(out/'math.o')
link=(parent/'link-argv.txt').read_text().splitlines()
assert link[0]==tool+'ld' and link.count(old_object)==1
link[link.index(old_object)]=str(out/'math.o')
for option,name in (('-T','program.ld'),('-Map','program.map'),('-o','program.elf')):
 assert link.count(option)==1;link[link.index(option)+1]=str(out/name)
(out/'program.ld').write_bytes((parent/'program.ld').read_bytes())
subprocess.run(command,check=True);subprocess.run(link,check=True)
assert (out/'program.elf').read_bytes()==baseline['program.elf'],'ENTIRE unchanged9f worker FIRST'
(out/'program.elf').rename(out/'unchanged-prefetch.elf')
(out/'math.o').rename(out/'unchanged-math.o')
command.insert(1,'-DPIONEER_Q38_QUEUED_Q4_BLOCK_IMAGE=1')
# NEW Q4-only outlined templates stay in the SAME owned service extent.
# Do not enlarge any section/stack/descriptor bounds to make a link pass.
layout=(parent/'program.ld').read_text()
anchor='*(.text.*PreparedQ4*)'
assert layout.count(anchor)==1
layout=layout.replace(anchor,anchor+'\n'
 '*(.text._ZN23PioneerQ38ExactQ4Affine12PanelEmitterI28PioneerQ38ExactQ4WorkEmitterE6ConfigEjj*)\n'
 '*(.text._ZN23PioneerQ38ExactQ4Affine12PanelEmitterI28PioneerQ38ExactQ4WorkEmitterE5DrainEv*)\n'
 '*(.text._ZN24PioneerQ38ExactQ4Primary5SetupI28PioneerQ38ExactQ4WorkEmitterE*)')
(out/'program.ld').write_text(layout)
subprocess.run(command,check=True);subprocess.run(link,check=True)
(out/'compile-math-argv.txt').write_text('\n'.join(command)+'\n')
(out/'link-argv.txt').write_text('\n'.join(link)+'\n')
elf=out/'program.elf'
assert not subprocess.check_output([tool+'nm','-u',str(elf)]).strip()
symbols=subprocess.check_output([tool+'nm','-n',str(elf)],text=True)
assert not re.search(r'^[0-9a-f]+ [bBdDgGsS] ',symbols,re.M),'no writable globals/GOT'
dis=subprocess.check_output([tool+'objdump','-d',str(elf)],text=True)
assert not re.search(r'\b(?:divu?|remu?|mulh[su]*|fdiv\.[sd]|fsqrt\.[sd]|fmadd|fmsub|fnmadd|fnmsub|fcvt\.wu?\.s|ebreak)\b',dis)
frames=[row.split('\t') for row in (out/'math.su').read_text().splitlines()]
assert frames and all(len(row)==3 and row[2]=='static' and int(row[1])<=4096 for row in frames)
sys.path.insert(0,str(here/'tests'))
from pioneer_q38_vector_q4_relocations import BASES
rows=subprocess.check_output([tool+'objdump','-h',str(elf)],text=True).splitlines()
for i,row in enumerate(rows):
 m=re.match(r'\s*\d+\s+(\S+)\s+([0-9a-fA-F]+)\s+',row)
 if m and 'ALLOC' in rows[i+1]:assert m[1][1:] in BASES,row
for section in BASES:
 target=out/(section+'.bin')
 subprocess.run([tool+'objcopy','-O','binary','-j','.'+section,str(elf),str(target)],check=True)
 data=target.read_bytes();assert data and not len(data)%4
 print(section,len(data),hashlib.sha256(data).hexdigest(),'EXACT' if data==baseline[target.name] else 'NEW_UNAUDITED',flush=True)
(out/'program.symbols').write_text(symbols);(out/'program.disassembly').write_text(dis)
print('INSPECT_ONLY_UNQUALIFIED_WHOLE_WORKER '+hashlib.sha256(elf.read_bytes()).hexdigest(),flush=True)
print('NO relocation/body audit,complete-worker or failure qualification,native integration,device or speed grant',flush=True)
