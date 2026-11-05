#!/usr/bin/env python3
"""BUILD/INSPECT ONLY: bounded T0 instruction-prefetch whole worker candidate."""
import hashlib
import os
import pathlib
import re
import subprocess
import sys
from generate_qwen38_token_programs import authenticated,pins
PINS=(
 ('boot.bin',4,'a12af9d63593a1f882750a147dd7a511ac476c5c7a55526bd4d51c685173991c'),
 ('globalworker.bin',9720,'6b04a57faf2e6a17ee7ac9d9881420ed56a97b3e2b65bcbf0e7f215e093cb533'),
 ('precise.bin',8392,'32c078029e1c74ec4bd9744ea494433f5003f63a936df05445d3066c722a7ecc'),
 ('dense.bin',10332,'d1749707ffe803e28dced9c7d065e763f125009678ace33b4d468109984869ad'),
 ('ffn.bin',16456,'e6533bc2035a251dbc0f4aad1da9d5222fcf8649ff2577db375843dc746014b7'),
 ('service.bin',17512,'2f5374449725d034c1e4eb0d685926f9046541bc1377ace57ed2939982f01610'),
 ('fetch.bin',1292,'a74f8f6303787360f35b588e22746074adc9bcc780ae5a274be25331f765f4c6'),
 ('affine.bin',4680,'a8f650e58052aba96187bcbee08c25a24de4b8aa8d92c5bb7c34ccfeeca4682d'),
 ('text.bin',7844,'fa783006d1943e886d777982f4aecc93fc0c321937e15f6d87755a14ddc07e00'),
 ('gdn.bin',35944,'7ae46e0cea089f93ae7662d2033e469393c4a39859dbc3745d40c456033944c3'),
 ('program.elf',154332,'9f1518398a3145ee30254ce06cd291403366070815dfdcbe26635f0340265f26'),
) # Main/clean BUILD identity ONLY;not hardware/native/model qualification.

if not __debug__ or os.environ.get('PYTHONOPTIMIZE'):
    raise SystemExit('Whole worker checks required')
inspect=len(sys.argv)==4 and sys.argv[3]=='--inspect-candidate'
assert len(sys.argv)==3 or inspect
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
parent,out=map(pathlib.Path,sys.argv[1:3])
assert parent.is_absolute() and parent.is_dir() and not parent.is_symlink()
assert out.is_absolute() and not out.exists() and not out.is_symlink()
here=pathlib.Path(__file__).resolve().parent
sys.path.insert(0,str(here/'tests'))
from pioneer_q38_prepared_q4_relocations import audit
from pioneer_q38_vector_q4_relocations import BASES
records=pins(here/'build_qwen38_two_product_q4_worker.py','REPLAY_PINS')
snapshot=authenticated(parent,records)
out.mkdir(mode=0o700)
tool='/Volumes/HaikuBuildLocal/generated.riscv64/cross-tools-riscv64/bin/riscv64-unknown-haiku-'
command=(parent/'unchanged-prepared/compile-global-argv.txt').read_text().splitlines()
assert command[0]==tool+'g++' and command[-4]=='-c' and command[-2]=='-o'
assert command[-3].endswith('/blackhole_qwen38_global_worker.cpp')
command[-3]=str(here/'blackhole_qwen38_global_worker.cpp')
old_object=command[-1];command[-1]=str(out/'global.o')
assert command.count('-DPIONEER_Q38_PREPARED_EXACT_Q4_WORKER_IMAGE=1')==1
link=(parent/'link-argv.txt').read_text().splitlines()
assert link[0]==tool+'ld' and link.count(old_object)==1
link[link.index(old_object)]=str(out/'global.o')
for option,name in (('-T','program.ld'),('-Map','program.map'),('-o','program.elf')):
    assert link.count(option)==1;link[link.index(option)+1]=str(out/name)
(out/'program.ld').write_bytes((parent/'program.ld').read_bytes())
subprocess.run(command,check=True);subprocess.run(link,check=True)
assert (out/'program.elf').read_bytes()==snapshot['program.elf'],'ENTIRE unchanged replay13 worker FIRST'
(out/'program.elf').rename(out/'unchanged-replay.elf')
(out/'global.o').rename(out/'unchanged-global.o')
command.insert(1,'-DPIONEER_Q38_INSTRUCTION_PREFETCH_WORKER_IMAGE=1')
subprocess.run(command,check=True);subprocess.run(link,check=True)
(out/'compile-global-argv.txt').write_text('\n'.join(command)+'\n')
(out/'link-argv.txt').write_text('\n'.join(link)+'\n')
elf=out/'program.elf'
assert not subprocess.check_output([tool+'nm','-u',str(elf)]).strip()
symbols=subprocess.check_output([tool+'nm','-n',str(elf)],text=True)
assert not re.search(r'^[0-9a-f]+ [bBdDgGsS] ',symbols,re.M)
dis=subprocess.check_output([tool+'objdump','-d',str(elf)],text=True)
assert not re.search(r'\b(?:divu?|remu?|mulh[su]*|fdiv\.[sd]|fsqrt\.[sd]|fmadd|fmsub|fnmadd|fnmsub|fcvt\.wu?\.s|ebreak)\b',dis)
reserved=[row for row in dis.splitlines() if re.search(r'\bs11\b',row)]
assert len(reserved)==2 and sum(bool(re.search(r'\bmv\s+s11,\w+\s*$',row)) for row in reserved)==1
assert sum(bool(re.search(r'\bmv\s+\w+,s11\s*$',row)) for row in reserved)==1
frames=[row.split('\t') for row in (out/'global.su').read_text().splitlines()]
assert frames and all(len(row)==3 and row[2]=='static' and int(row[1])<=2048 for row in frames)
rows=subprocess.check_output([tool+'objdump','-h',str(elf)],text=True).splitlines()
for i,row in enumerate(rows):
    m=re.match(r'\s*\d+\s+(\S+)\s+([0-9a-fA-F]+)\s+',row)
    if m and 'ALLOC' in rows[i+1]:assert m[1][1:] in BASES,row
for section in BASES:
    target=out/(section+'.bin')
    subprocess.run([tool+'objcopy','-O','binary','-j','.'+section,str(elf),str(target)],check=True)
    if section!='globalworker':assert target.read_bytes()==snapshot[target.name],section
assert 0<(out/'globalworker.bin').stat().st_size<=0x10000
assert max(BASES[s]+(out/(s+'.bin')).stat().st_size for s in BASES)==0xc0c68,'EXACT prefetch upper bound'
audit(parent/'unchanged-prepared/unchanged-original',out,tool,two_product=True)
refusals=[('zero-prefetch',['-UPIONEER_Q38_INSTRUCTION_PREFETCH_WORKER_IMAGE','-DPIONEER_Q38_INSTRUCTION_PREFETCH_WORKER_IMAGE=0']),
          ('missing-prepared',['-UPIONEER_Q38_PREPARED_EXACT_Q4_WORKER_IMAGE']),
          ('zero-prepared',['-UPIONEER_Q38_PREPARED_EXACT_Q4_WORKER_IMAGE','-DPIONEER_Q38_PREPARED_EXACT_Q4_WORKER_IMAGE=0']),
          ('fast-math',['-ffast-math'])]
for macro in ('_KERNEL_MODE','PIONEER_DRIVER_BUILD_TAG','TT_PIONEER_DRIVER_RELOAD',
              'PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_FULL_MODEL_CPU_REFERENCE','PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE',
              'PIONEER_Q38_FULL_OFFLOAD_CLIENT','PIONEER_Q38_FULL_OFFLOAD_NATIVE'):
    refusals.append((macro+'-defined0',['-D'+macro+'=0']))
for label,extra in refusals:
    result=subprocess.run([*command[:command.index('-c')],*extra,'-fsyntax-only',command[-3]],stdout=subprocess.DEVNULL,stderr=subprocess.PIPE,text=True)
    assert result.returncode and 'Instruction-prefetch IMAGE requires' in result.stderr,label
(out/'role-refusals.txt').write_text(''.join('PASS '+label+'\n' for label,_ in refusals))
(out/'program.symbols').write_text(symbols);(out/'program.disassembly').write_text(dis)
subprocess.run([sys.executable,str(here/'tests/pioneer_q38_gdn_rendezvous_csr_gate.py'),str(out/'program.disassembly')],check=True)
observed=[]
for name in (*(s+'.bin' for s in BASES),'program.elf'):
    data=(out/name).read_bytes();record=(name,len(data),hashlib.sha256(data).hexdigest());observed.append(record);print(record,flush=True)
if not inspect:assert tuple(observed)==PINS
print('BUILD_INSPECT_ONLY T0prefetch distinct global worker;old replay13 whole ELF FIRST/all9 other sections unchanged;'+str(len(refusals))+' role refusals;NO_NATIVE_PHYSICAL_OR_TPS_GRANT',flush=True)
