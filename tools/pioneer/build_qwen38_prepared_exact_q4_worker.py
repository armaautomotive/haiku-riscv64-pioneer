#!/usr/bin/env python3
"""BUILD ONLY complete prepared-Q4 worker;no native/source/ROOT/physical grant."""
import hashlib,os,pathlib,re,runpy,subprocess,sys
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parent/'tests'))
from pioneer_q38_prepared_q4_relocations import audit
if not __debug__ or os.environ.get('PYTHONOPTIMIZE'):raise SystemExit('Refuse optimized Python')
args=sys.argv[1:];inspect='--inspect-candidate' in args
if inspect:assert args.count('--inspect-candidate')==1;args.remove('--inspect-candidate')
PINS=(
 ('boot.bin',4,'a12af9d63593a1f882750a147dd7a511ac476c5c7a55526bd4d51c685173991c'),
 ('globalworker.bin',9504,'35bb8e77470d792b6a040b4aed99b2c966f2a7fb8e1a16a16deb3f9fc4bd5496'),
 ('precise.bin',8392,'32c078029e1c74ec4bd9744ea494433f5003f63a936df05445d3066c722a7ecc'),
 ('dense.bin',10332,'d1749707ffe803e28dced9c7d065e763f125009678ace33b4d468109984869ad'),
 ('fetch.bin',1292,'a74f8f6303787360f35b588e22746074adc9bcc780ae5a274be25331f765f4c6'),
 ('text.bin',7844,'fa783006d1943e886d777982f4aecc93fc0c321937e15f6d87755a14ddc07e00'),
 ('affine.bin',4680,'a8f650e58052aba96187bcbee08c25a24de4b8aa8d92c5bb7c34ccfeeca4682d'),
 ('ffn.bin',16456,'e6533bc2035a251dbc0f4aad1da9d5222fcf8649ff2577db375843dc746014b7'),
 ('service.bin',17624,'9630de58660a69d227357d447bb230329ed2e39f32cbb59b855756f08b3b698c'),
 ('gdn.bin',35944,'7ae46e0cea089f93ae7662d2033e469393c4a39859dbc3745d40c456033944c3'),
 ('program.elf',154308,'b283d788a682681f8994058ab51b19b2b1e79f72945d7e839c9cdd81737ff9c3'),
) # Main+independent clean BUILD identity,NOT a native/source/ROOT grant.
assert len(args)==2 and (inspect or PINS)
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
sdk,out=map(pathlib.Path,args);here=pathlib.Path(__file__).resolve().parent
assert sdk.is_absolute() and sdk.is_dir() and out.is_absolute() and not out.exists() and not out.is_symlink()
out.mkdir(mode=0o700);saved=sys.argv[:]
try:
 sys.argv=[str(here/'build_qwen38_exact_q4_work_worker.py'),str(sdk),str(out/'unchanged-original')]
 old=runpy.run_path(sys.argv[0],run_name='__main__')
finally:sys.argv=saved
control=out/'unchanged-original';tool=old['tool']
assert hashlib.sha256((control/'program.elf').read_bytes()).hexdigest()=='8a7c6543e59a4067cee72d67843ca2d33119c307a8277bd14a70cb80f2f41e2f'
def run(*values):subprocess.run(list(map(str,values)),check=True)
commands={
 'math':(control/'compile-math-argv.txt','blackhole_qwen38_prepared_exact_q4_math.cpp','PIONEER_Q38_PREPARED_EXACT_Q4_IMAGE'),
 'global':(control/'unchanged-vector/unchanged-wide/unchanged-twin/unchanged-batched/compile-global-argv.txt','blackhole_qwen38_global_worker.cpp','PIONEER_Q38_PREPARED_EXACT_Q4_WORKER_IMAGE'),
}
replacements={}
for label,(path,source,flag) in commands.items():
 command=path.read_text().splitlines();original=command[command.index('-o')+1]
 command[command.index('-o')+1]=str(out/('unchanged-'+label+'.o'));run(*command)
 assert (out/('unchanged-'+label+'.o')).read_bytes()==pathlib.Path(original).read_bytes(),'ENTIRE old '+label+' object FIRST'
 command[command.index('-o')+1]=str(out/(label+'.o'));command[command.index('-c')+1]=str(here/source)
 command=[command[0],'-D'+flag+'=1',*command[1:]];run(*command)
 (out/('compile-'+label+'-argv.txt')).write_text('\n'.join(command)+'\n');replacements[original]=str(out/(label+'.o'))
layout=(control/'program.ld').read_text();service='*(.text.pioneer_q38_resident_service)'
assert layout.count(service)==1
layout=layout.replace(service,service+'\n\t\t*(.text.*PreparedQ4*)'
 '\n\t\t*(.text._ZN25PioneerQ38NativeBfpMatvec5SetupI28PioneerQ38ExactQ4WorkEmitterE*)')
(out/'program.ld').write_text(layout)
link=[replacements.get(value,value) for value in old['link']]
assert all(sum(value==target for value in link)==1 for target in replacements.values())
for option,name in (('-T','program.ld'),('-Map','program.map'),('-o','program.elf')):link[link.index(option)+1]=str(out/name)
run(*link);(out/'link-argv.txt').write_text('\n'.join(link)+'\n')
elf=out/'program.elf';assert not subprocess.check_output([tool+'nm','-u',str(elf)]).strip()
symbols=subprocess.check_output([tool+'nm','-n',str(elf)],text=True)
assert not re.search(r'^[0-9a-f]+ [bBdDgGsS] ',symbols,re.M),'no writable globals/GOT'
dis=subprocess.check_output([tool+'objdump','-d',str(elf)],text=True)
assert not re.search(r'\b(?:divu?|remu?|mulh[su]*|fdiv\.[sd]|fsqrt\.[sd]|fmadd|fmsub|fnmadd|fnmsub|fcvt\.wu?\.s|ebreak)\b',dis)
reserved=[row for row in dis.splitlines() if re.search(r'\bs11\b',row)]
assert len(reserved)==2 and sum(bool(re.search(r'\bmv\s+s11,\w+\s*$',row)) for row in reserved)==1
assert sum(bool(re.search(r'\bmv\s+\w+,s11\s*$',row)) for row in reserved)==1
for label in commands:
 frames=[row.split('\t') for row in (out/(label+'.su')).read_text().splitlines()]
 assert frames and all(len(row)==3 and row[2]=='static' and int(row[1])<=4096 for row in frames)
sections=old['sections'];rows=subprocess.check_output([tool+'objdump','-h',str(elf)],text=True).splitlines()
for i,row in enumerate(rows):
 m=re.match(r'\s*\d+\s+(\S+)\s+([0-9a-fA-F]+)\s+',row)
 if m and 'ALLOC' in rows[i+1]:assert m[1][1:] in sections,row
for section in sections:run(tool+'objcopy','-O','binary','-j','.'+section,elf,out/(section+'.bin'))
for section,limit in (('globalworker',0x10000),('fetch',0x700),('affine',0x1500),('text',0x2000),('service',0x6000)):
 assert 0<(out/(section+'.bin')).stat().st_size<=limit,(section,(out/(section+'.bin')).stat().st_size)
for section in ('boot','precise','dense'):assert (out/(section+'.bin')).read_bytes()==(control/(section+'.bin')).read_bytes(),section
audit(control,out,tool)
(out/'program.symbols').write_text(symbols);(out/'program.disassembly').write_text(dis)
run(sys.executable,here/'tests/pioneer_q38_gdn_rendezvous_csr_gate.py',out/'program.disassembly')
observed=[]
for name in (*(s+'.bin' for s in sections),'program.elf'):
 data=(out/name).read_bytes();assert data and not len(data)%4
 item=(name,len(data),hashlib.sha256(data).hexdigest());observed.append(item);print(item,flush=True)
if not inspect:assert tuple(observed)==PINS
print('BUILD_ONLY complete prepared-Q4 primary+bounded-group-correction worker;NO execution/native/ROOT/source/physical/model/TPS grant',flush=True)
