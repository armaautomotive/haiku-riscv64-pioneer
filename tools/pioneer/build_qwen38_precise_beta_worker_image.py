#!/usr/bin/env python3
"""Isolated QGW3 precise-beta worker candidate; BUILD ONLY, never install."""
import hashlib
import os
import pathlib
import re
import runpy
import struct
import subprocess
import sys

inspect='--inspect-candidate' in sys.argv
if inspect:
    assert sys.argv.count('--inspect-candidate')==1
    sys.argv.remove('--inspect-candidate')
assert len(sys.argv)==3 and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
sdk,out=map(pathlib.Path,sys.argv[1:]);here=pathlib.Path(__file__).resolve().parent
assert sdk.is_absolute() and out.is_absolute() and not out.exists() and not out.is_symlink()
out.mkdir(mode=0o700);saved=sys.argv[:]
try:
    sys.argv=[str(here/'build_qwen38_pushed_worker_image.py'),str(sdk),str(out/'unchanged-pushed')]
    ancestor=runpy.run_path(sys.argv[0],run_name='__main__')
finally:
    sys.argv=saved
# Ancestor rebuilt and authenticated EVERY old literal. Only the GDN input
# capsule is recompiled; old ROOT, matrix, activation, conv and state stay.
mixed=ancestor['old']['parent']['old']
unit=next(u for u in mixed['units'] if u[1]=='gdn_input')
assert unit[0]=='blackhole_qwen38_gdn_input.cpp'
def run(*args):subprocess.run(list(map(str,args)),check=True)
cross=ancestor['cross'];flags=ancestor['flags']
run(str(cross)+'-g++',*flags,unit[3],*mixed['fixed'],*unit[2],
    '-DPIONEER_Q38_GDN_PRECISE_BETA_IMAGE=1','-DPIONEER_Q38_SIGMOID_PRECISION_IMAGE=1',
    '-c',here/unit[0],'-o',out/'gdn_input.o')
control=out/'unchanged-pushed';precise=control/'unchanged-precise';math=precise/'unchanged-global/qualified-math'
(out/'program.ld').write_text((control/'program.ld').read_text())
objects=[out/'gdn_input.o' if n=='gdn_input' else math/(n+'.o') for n in ancestor['old']['parent']['units']]
assert objects.count(out/'gdn_input.o')==1
run(str(cross)+'-ld','-m','elf32lriscv','--no-relax','--gc-sections','-T',out/'program.ld',
    precise/'start.o',control/'global.o',precise/'precise.o',*objects,'-Map',out/'program.map','-o',out/'program.elf')
sections=ancestor['sections']
for section in sections:run(str(cross)+'-objcopy','-O','binary','-j','.'+section,out/'program.elf',out/(section+'.bin'))
symbols=subprocess.check_output([str(cross)+'-nm','-n',str(out/'program.elf')],text=True)
assert not subprocess.check_output([str(cross)+'-nm','-u',str(out/'program.elf')],text=True).strip()
assert not re.search(r'^[0-9a-f]+ [bBdDgGsS] ',symbols,re.M)
dis=subprocess.check_output([str(cross)+'-objdump','-d',str(out/'program.elf')],text=True)
assert not re.search(r'\b(?:divu?|remu?|mulh[su]*|fdiv\.[sd]|fsqrt\.[sd]|fmadd|fmsub|fnmadd|fnmsub|fcvt\.wu?\.s|ebreak)\b',dis)
reserved=[line for line in dis.splitlines() if re.search(r'\bs11\b',line)]
assert len(reserved)==2 and sum(bool(re.search(r'\bmv\s+s11,\w+\s*$',x)) for x in reserved)==1
assert sum(bool(re.search(r'\bmv\s+\w+,s11\s*$',x)) for x in reserved)==1
frames=[int(line.split('\t')[1]) for line in (out/'gdn_input.su').read_text().splitlines()]
assert frames and max(frames)<=4096
rows=subprocess.check_output([str(cross)+'-objdump','-h',str(out/'program.elf')],text=True).splitlines()
for i,row in enumerate(rows):
    match=re.match(r'\s*\d+\s+(\S+)\s+([0-9a-fA-F]+)\s+',row)
    if match and 'ALLOC' in rows[i+1]:assert match[1][1:] in sections,row
for section in sections:
    if section not in ('gdn','globalworker'):
        assert (out/(section+'.bin')).read_bytes()==(control/(section+'.bin')).read_bytes(),section
# The unchanged global.o calls compiler-emitted memcpy/memset bodies in
# .gdn. Replacing the input capsule moves these bodies, so precisely THREE
# JALR immediates relocate. Audit decoded targets AND unchanged callee bytes;
# do not accept arbitrary changed instructions or relabel all bytes equal.
def sized_symbols(elf):
    result={}
    for line in subprocess.check_output([str(cross)+'-nm','-S','-n',str(elf)],text=True).splitlines():
        parts=line.split()
        if len(parts)==4:result[int(parts[0],16)]=(parts[3],int(parts[1],16))
    return result
original=struct.unpack('<2282I',(control/'globalworker.bin').read_bytes())
candidate=struct.unpack('<2282I',(out/'globalworker.bin').read_bytes())
changed=[i for i in range(len(original)) if original[i]!=candidate[i]]
assert [0x20000+4*i for i in changed]==[0x2018c,0x20fa4,0x21828]
maps=[sized_symbols(control/'program.elf'),sized_symbols(out/'program.elf')]
gdn=[(control/'gdn.bin').read_bytes(),(out/'gdn.bin').read_bytes()]
for index in changed:
    targets=[]
    assert original[index-1]==candidate[index-1] and original[index-1]&0xfff==0x097
    for side,words in enumerate((original,candidate)):
        assert words[index]&0xfffff==0x080e7 # jalr ra,immediate(ra),ONLY immediate may change
        high=words[index-1]&0xfffff000;high=high-(1<<32) if high>>31 else high
        low=words[index]>>20;low=low-4096 if low>>11 else low
        destination=0x20000+(index-1)*4+high+low
        name,size=maps[side][destination]
        assert name in ('memcpy','memset') and size and destination>=0xb8000
        offset=destination-0xb8000;body=gdn[side][offset:offset+size]
        assert len(body)==size;targets.append((name,size,body))
    assert targets[0]==targets[1], 'relocated call must still reach the SAME exact helper instructions'
    print('AUDITED_SAME_HELPER_RELOCATION',hex(0x20000+index*4),targets[0][0],targets[0][1])
(out/'program.symbols').write_text(symbols);(out/'program.disassembly').write_text(dis)
run(sys.executable,here/'tests/pioneer_q38_gdn_rendezvous_csr_gate.py',out/'program.disassembly')
observed=[]
for name in (*(s+'.bin' for s in sections),'program.elf'):
    data=(out/name).read_bytes();assert data and not len(data)%4
    observed.append((name,len(data),hashlib.sha256(data).hexdigest()));print(observed[-1])
PINS=(
    ('boot.bin',4,'a12af9d63593a1f882750a147dd7a511ac476c5c7a55526bd4d51c685173991c'),
    ('globalworker.bin',9128,'e8627b672291439f1b6985e000e91dab8c7a68dfa56d004d223b1e9667a6c6fc'),
    ('precise.bin',8392,'32c078029e1c74ec4bd9744ea494433f5003f63a936df05445d3066c722a7ecc'),
    ('dense.bin',10332,'d1749707ffe803e28dced9c7d065e763f125009678ace33b4d468109984869ad'),
    ('text.bin',5984,'e1bd0788b82ff4f3ad58e1d527199ba4bc23ecd55936f7940d3f669c617aa419'),
    ('affine.bin',5360,'47c52fee6ccfb0a8847fe17a9d43d4016e7cfc6c68cc1427673f6869cd287b78'),
    ('ffn.bin',16456,'23ce6cc2dc07aa445b7babcac12a068dc3542001329cd24831e8e6749ff3a16f'),
    ('service.bin',9888,'7d5a42a087bc6bf9d24319583f3655ae344828a51b1092154f7b0567b0fdb187'),
    ('gdn.bin',35408,'cbe3faf60ef0b3a0544b27b29f99d62eb36c7d0ac5b835e337c5501661ed589c'),
    ('program.elf',139528,'1f49e9740d8d9045de6c8835f0f8f1738210ba0413d87f607f97cf98da74539e'),
) # Independent main/clean emitted artifacts; NOT runtime/physical/Work proof.
if not inspect:assert PINS and tuple(observed)==PINS,'Unqualified candidate: inspect/reproduce/freeze first'
print('BUILD_ONLY precise beta/old literals independently rebuilt/non-GDN bytes identical except3 audited SAME-helper call relocations/max input frame'+str(max(frames))+
      ';NO emitted runtime/full numerical/native/physical/Work/TPS qualification;DO NOT INSTALL')
