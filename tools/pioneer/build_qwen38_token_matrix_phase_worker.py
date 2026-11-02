#!/usr/bin/env python3
"""Complete current returning worker plus Q4-job sampler BUILD only."""
import hashlib
import os
import pathlib
import re
import runpy
import subprocess
import sys

PINS = (
    ('boot.bin', 4, 'a12af9d63593a1f882750a147dd7a511ac476c5c7a55526bd4d51c685173991c'),
    ('globalworker.bin', 9232, '521501cea945a13f9c067bb791d22137db6e1ee5c4cd7d70464c786033f04f2e'),
    ('precise.bin', 8392, '32c078029e1c74ec4bd9744ea494433f5003f63a936df05445d3066c722a7ecc'),
    ('dense.bin', 10332, 'd1749707ffe803e28dced9c7d065e763f125009678ace33b4d468109984869ad'),
    ('fetch.bin', 1292, 'cb564c312ec5c798a1e2e7e2f8b08d94e77b2d4734386332d1f88149a711faba'),
    ('text.bin', 6768, 'dcc7e7201e7aefd70d50ad11db6a2cce97a296dee00e6f3d24e63656a2f40fcb'),
    ('affine.bin', 5360, '96d69025bc3b87ebad872089be199308a4f2dcdeebb0c5d01da0b67d49cccbc9'),
    ('ffn.bin', 16456, '23ce6cc2dc07aa445b7babcac12a068dc3542001329cd24831e8e6749ff3a16f'),
    ('service.bin', 11136, '244286c80a33a71d1ca1af87bd73b7d2320cac74a5bffb6878256b9bc62aeecb'),
    ('gdn.bin', 35944, 'c2824d4822c69867879b09fac67f9f9849cf5f7202c1beca668584976eaf4e9c'),
    ('program.elf', 145576, 'd5ba1c3af9ac040cc1f6f4a8e8d08228a0351f3fadd604ab25f8e5bcb7da93ff'),
)  # Independent whole BUILD identity only,not execution authority.
args=sys.argv[1:];inspect='--inspect-candidate' in args
if inspect:
    assert args.count('--inspect-candidate')==1;args.remove('--inspect-candidate')
assert len(args)==2 and (inspect or PINS)
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
sdk,out=map(pathlib.Path,args);here=pathlib.Path(__file__).resolve().parent
assert sdk.is_absolute() and out.is_absolute() and not out.exists() and not out.is_symlink()
out.mkdir(mode=0o700);saved=sys.argv[:]
try:
    sys.argv=[str(here/'build_qwen38_q4_constant_issue_worker_image.py'),str(sdk),str(out/'unchanged-constant-q4')]
    parent=runpy.run_path(sys.argv[0],run_name='__main__')
finally:sys.argv=saved
control=out/'unchanged-constant-q4';tool=parent['tool']
assert hashlib.sha256((control/'program.elf').read_bytes()).hexdigest()=='e9ef5c53dbb3f8a924920c5dac261a884bbe33c3168d90cfd168e02fa96d8397'
command=(control/'compile-math-argv.txt').read_text().splitlines();original=command[command.index('-o')+1]
matrix=out/'matrix-phase-program.o'
command[command.index('-c')+1]=str(here/'blackhole_qwen38_token_matrix_phase_math.cpp')
command[command.index('-o')+1]=str(matrix)
command=[command[0],'-DPIONEER_Q38_TOKEN_MATRIX_PHASE_IMAGE=1',*command[1:]]
subprocess.run(command,check=True);(out/'compile-math-argv.txt').write_text('\n'.join(command)+'\n')
# Existing service ROM has space;do NOT grow text/affine/fetch views or
# move helpers into stack/operands. Future native owner still owes its own
# code DATA/complete original histories/held readout before deployment.
script=(control/'program.ld').read_text()
needle='\t.service 0x68000 : {\n';assert script.count(needle)==1
script=script.replace(needle,needle+'\t\t*(.text.pioneer_q38_token_matrix_phase*)\n')
(out/'program.ld').write_text(script)
link=(control/'link-argv.txt').read_text().splitlines();assert link.count(original)==1
link=[str(matrix) if p==original else p for p in link]
for option,name in (('-T','program.ld'),('-Map','program.map'),('-o','program.elf')):link[link.index(option)+1]=str(out/name)
subprocess.run(link,check=True);(out/'link-argv.txt').write_text('\n'.join(link)+'\n')
elf=out/'program.elf';assert not subprocess.check_output([tool+'nm','-u',str(elf)]).strip()
symbols=subprocess.check_output([tool+'nm','-n',str(elf)],text=True)
names=subprocess.check_output([tool+'nm','-nC',str(elf)],text=True)
assert not re.search(r'^[0-9a-f]+ [bBdDgGsS] ',symbols,re.M)
for name in ('PioneerQ38TokenMatrixPhase::','MatrixPhaseClock(','PublishMatrixPhase(',
             'PioneerQ38Q4ConstantTransform<','TwinBatchTilePrefetchStage(','q38_compute_once'):
    assert name in names,name
dis=subprocess.check_output([tool+'objdump','-d',str(elf)],text=True)
assert not re.search(r'\b(?:divu?|remu?|mulh[su]*|fdiv\.[sd]|fsqrt\.[sd]|fmadd|fmsub|fnmadd|fnmsub|fcvt\.wu?\.s|ebreak)\b',dis)
csrs=[]
for line in dis.splitlines():
    m=re.match(r'\s*([0-9a-f]+):\s+([0-9a-f]{8})\s',line)
    if not m:continue
    address,op=map(lambda x:int(x,16),m.groups())
    if op&127!=0x73:continue
    assert (op>>12)&7==2 and (op>>15)&31==0 and op>>20 in (0xb00,0xb80) and (op>>7)&31
    csrs.append(address)
assert len(csrs)==6 and sum(0x68000<=a<0x6e000 for a in csrs)==3,csrs
frames=[row.split('\t') for row in matrix.with_suffix('.su').read_text().splitlines()]
assert frames and all(len(r)==3 and r[2]=='static' and int(r[1])<=4096 for r in frames)
sections=parent['sections'];table=subprocess.check_output([tool+'objdump','-h',str(elf)],text=True).splitlines()
for i,row in enumerate(table):
    m=re.match(r'\s*\d+\s+(\S+)\s+([0-9a-f]+)\s+',row)
    if m and 'ALLOC' in table[i+1]:assert m[1][1:] in sections,row
for section in sections:subprocess.run([tool+'objcopy','-O','binary','-j','.'+section,str(elf),str(out/(section+'.bin'))],check=True)
for section,limit in (('text',0x2000),('affine',0x1500),('fetch',0x700),('service',0x6000)):
    assert 0<(out/(section+'.bin')).stat().st_size<=limit
for section in ('boot','precise','dense','ffn','gdn'):
    assert (out/(section+'.bin')).read_bytes()==(control/(section+'.bin')).read_bytes(),section
(out/'program.symbols').write_text(symbols);(out/'program.disassembly').write_text(dis)
observed=[]
for name in (*(s+'.bin' for s in sections),'program.elf'):
    data=(out/name).read_bytes();assert data and len(data)%4==0
    item=(name,len(data),hashlib.sha256(data).hexdigest());observed.append(item);print(item,flush=True)
if not inspect:assert tuple(observed)==PINS
print('BUILD_ONLY complete constant-Q4 returning worker/unchanged parent e9ef FIRST/three extra READ-only clock CSR sites/'
      'per-Q4-job inclusive fetch,decode,matrix+affine DATA/unchanged fixed code+stack+runtime extents/'
      'NO native owner,readout,physical,TPS,quality,Work or deployment grant',flush=True)
