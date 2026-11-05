#!/usr/bin/env python3
"""Whole returning corrected-Q4 worker BUILD ONLY;no native/install/GPU authority."""
import hashlib,os,pathlib,re,runpy,subprocess,sys
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parent/'tests'))
from pioneer_q38_exact_q4_work_relocations import audit
if not __debug__ or os.environ.get('PYTHONOPTIMIZE'):raise SystemExit('Refuse optimized Python')
args=sys.argv[1:];inspect='--inspect-candidate' in args
if inspect:assert args.count('--inspect-candidate')==1;args.remove('--inspect-candidate')
PINS=(
 ('boot.bin',4,'a12af9d63593a1f882750a147dd7a511ac476c5c7a55526bd4d51c685173991c'),
 ('globalworker.bin',9232,'837b00298c9852c24d395a562afc71035f4ca79e3d3b6918ac5625051bbe6f66'),
 ('precise.bin',8392,'32c078029e1c74ec4bd9744ea494433f5003f63a936df05445d3066c722a7ecc'),
 ('dense.bin',10332,'d1749707ffe803e28dced9c7d065e763f125009678ace33b4d468109984869ad'),
 ('fetch.bin',1292,'a74f8f6303787360f35b588e22746074adc9bcc780ae5a274be25331f765f4c6'),
 ('text.bin',7844,'be123bd30706ccde16869bd9c800b808a77a8770a5794fff47880bdabaaf5923'),
 ('affine.bin',4680,'a8f650e58052aba96187bcbee08c25a24de4b8aa8d92c5bb7c34ccfeeca4682d'),
 ('ffn.bin',16456,'e6533bc2035a251dbc0f4aad1da9d5222fcf8649ff2577db375843dc746014b7'),
 ('service.bin',15196,'fd8b410c39c34a43fe64de244461e84f95086748e7524bc1624aa88b840c17f9'),
 ('gdn.bin',35944,'7ae46e0cea089f93ae7662d2033e469393c4a39859dbc3745d40c456033944c3'),
 ('program.elf',149880,'8a7c6543e59a4067cee72d67843ca2d33119c307a8277bd14a70cb80f2f41e2f'),
) # Main+independent clean WHOLE builds and actual-RV dual-job tests.
assert len(args)==2 and (inspect or PINS)
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
sdk,out=map(pathlib.Path,args);here=pathlib.Path(__file__).resolve().parent
assert sdk.is_absolute() and sdk.is_dir() and out.is_absolute() and not out.exists() and not out.is_symlink()
out.mkdir(mode=0o700);saved=sys.argv[:]
try:
 sys.argv=[str(here/'build_qwen38_vector_q4_metadata_worker.py'),str(sdk),str(out/'unchanged-vector')]
 parent=runpy.run_path(sys.argv[0],run_name='__main__')
finally:sys.argv=saved
control=out/'unchanged-vector';tool=parent['tool']
assert hashlib.sha256((control/'program.elf').read_bytes()).hexdigest()=='4b3dc0c96d3888b9c43af683f58678cc91cade7c67e955413bad8c5e16d52870'
def run(*command):subprocess.run(list(map(str,command)),check=True)
command=(control/'compile-math-argv.txt').read_text().splitlines();original=pathlib.Path(command[command.index('-o')+1])
command[command.index('-o')+1]=str(out/'unchanged-matrix.o');run(*command)
assert (out/'unchanged-matrix.o').read_bytes()==original.read_bytes(),'ENTIRE accepted matrix object FIRST'
matrix=out/'exact-q4-work.o';command[command.index('-o')+1]=str(matrix)
command[command.index('-c')+1]=str(here/'blackhole_qwen38_exact_q4_work_math.cpp')
command=[command[0],'-DPIONEER_Q38_EXACT_Q4_WORK_IMAGE=1',*command[1:]];run(*command)
(out/'compile-math-argv.txt').write_text('\n'.join(command)+'\n')
layout=(control/'program.ld').read_text()
service='*(.text.pioneer_q38_resident_service)';affine='*(.text.pioneer_q38_replay_job_setup)'
assert layout.count(service)==layout.count(affine)==1
# Explicit NEW arithmetic specialization and the now dual-format cold
# initializer stay inside EXISTING service/affine code extents. No larger
# code view or descriptor overlap is admitted;old builds use original LD.
layout=layout.replace(service,service+'\n\t\t*(.text._ZN26PioneerQ38ExactQ4Corrected3RunIN23PioneerQ38ExactQ4Affine12PanelEmitterI28PioneerQ38ExactQ4WorkEmitterEEEEbRT_*)')
layout=layout.replace(affine,affine+'\n\t\t*(.text._Z23PioneerQ38ReplayDstInitI18VectorPanelEmitterEvRT_bbbb*)')
(out/'program.ld').write_text(layout)
link=parent['link'][:];assert link.count(str(original))==1
link=[str(matrix) if value==str(original) else value for value in link]
for option,name in (('-T','program.ld'),('-Map','program.map'),('-o','program.elf')):link[link.index(option)+1]=str(out/name)
run(*link);(out/'link-argv.txt').write_text('\n'.join(link)+'\n')
elf=out/'program.elf';assert not subprocess.check_output([tool+'nm','-u',str(elf)]).strip()
symbols=subprocess.check_output([tool+'nm','-n',str(elf)],text=True);names=subprocess.check_output([tool+'nm','-nC',str(elf)],text=True)
assert not re.search(r'^[0-9a-f]+ [bBdDgGsS] ',symbols,re.M),'no writable globals/GOT'
for name in ('PioneerQ38ExactQ4Work(', 'TwinBatchTilePrefetchStage(', 'q38_compute_once','PioneerQ38VectorQ4Metadata::Block'):
 assert name in names,name
dis=subprocess.check_output([tool+'objdump','-d',str(elf)],text=True)
assert not re.search(r'\b(?:divu?|remu?|mulh[su]*|fdiv\.[sd]|fsqrt\.[sd]|fmadd|fmsub|fnmadd|fnmsub|fcvt\.wu?\.s|ebreak)\b',dis)
reserved=[line for line in dis.splitlines() if re.search(r'\bs11\b',line)]
assert len(reserved)==2 and sum(bool(re.search(r'\bmv\s+s11,\w+\s*$',r)) for r in reserved)==1
assert sum(bool(re.search(r'\bmv\s+\w+,s11\s*$',r)) for r in reserved)==1
frames=[line.split('\t') for line in matrix.with_suffix('.su').read_text().splitlines()]
assert frames and all(len(row)==3 and row[2]=='static' and int(row[1])<=4096 for row in frames)
sections=parent['sections'];rows=subprocess.check_output([tool+'objdump','-h',str(elf)],text=True).splitlines()
for i,line in enumerate(rows):
 m=re.match(r'\s*\d+\s+(\S+)\s+([0-9a-fA-F]+)\s+',line)
 if m and 'ALLOC' in rows[i+1]:assert m[1][1:] in sections,line
for section in sections:run(tool+'objcopy','-O','binary','-j','.'+section,elf,out/(section+'.bin'))
for section,limit in (('fetch',0x700),('affine',0x1500),('text',0x2000),('service',0x6000)):
 assert 0<(out/(section+'.bin')).stat().st_size<=limit,(section,(out/(section+'.bin')).stat().st_size)
for section in ('boot','precise','dense'):assert (out/(section+'.bin')).read_bytes()==(control/(section+'.bin')).read_bytes(),section
# EVERY old non-math byte,including the entire old service prefix, is
# unchanged except whole-target-verified symbolic calls/data addresses.
# The six exact appended arithmetic functions grant NO native extent.
audit(control,out,tool)
(out/'program.symbols').write_text(symbols);(out/'program.disassembly').write_text(dis)
run(sys.executable,here/'tests/pioneer_q38_gdn_rendezvous_csr_gate.py',out/'program.disassembly')
observed=[]
for name in (*(section+'.bin' for section in sections),'program.elf'):
 data=(out/name).read_bytes();assert data and not len(data)%4
 item=(name,len(data),hashlib.sha256(data).hexdigest());observed.append(item);print(item,flush=True)
if not inspect:assert tuple(observed)==PINS,'candidate not independently frozen'
print(('INSPECT_BUILD_ONLY' if inspect else 'BUILD_ONLY')+
 ' complete corrected-Q4 original-source worker;nonmath relocation audit PASS;NO native/physical/model/TPS authority',flush=True)
