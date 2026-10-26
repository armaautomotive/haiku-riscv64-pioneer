#!/usr/bin/env python3
"""NEW on-card recipe admission and complete following layer; NO native/device IO."""
import hashlib
import os
import pathlib
import re
import runpy
import struct
import subprocess
import sys

if len(sys.argv)!=3 or any(k.startswith('PIONEER_') for k in os.environ):
    raise SystemExit(2)
sdk,output=map(pathlib.Path,sys.argv[1:])
assert sdk.is_absolute() and output.is_absolute() and not output.exists() and not output.is_symlink()
here=pathlib.Path(__file__).resolve().parent
output.mkdir(mode=0o700)
saved=list(sys.argv)
try:
    sys.argv=[str(here/'build_qwen38_resident_advance_image.py'),str(sdk),str(output/'unchanged-advance')]
    base=runpy.run_path(sys.argv[0],run_name='__main__')
finally:
    sys.argv=saved
prior=output/'unchanged-advance';link=prior/'unchanged-link';layer=link/'unchanged-layer'
cross,flags=base['cross'],base['flags']
def run(*args):
    subprocess.run([str(a) for a in args],check=True)
run(str(cross)+'-g++',*flags,'-O2','-DPIONEER_Q38_RESIDENT_CONTINUE_IMAGE=1',
    '-c',here/'blackhole_qwen38_resident_continue.cpp','-o',output/'continue.o')
run(str(cross)+'-as','-march=rv32imf','-mabi=ilp32','-mno-relax',here/'blackhole_qwen38_resident_continue_start.S','-o',output/'start.o')
run(str(cross)+'-ld','-m','elf32lriscv','--no-relax','--gc-sections','-T',here/'blackhole_qwen38_resident_continue.ld',
    output/'start.o',layer/'root.o',*[layer/(n+'.o') for n in ('matrix','norm','ffn','gather')],
    layer/'unchanged-output/unchanged-cohort/root_conv.o',link/'link.o',
    *[prior/(n+'.o') for n in ('advance','advance_matrix','advance_gather')],output/'continue.o',
    '-Map',output/'program.map','-o',output/'program.elf')
sections=('boot','text','conv','matrix','cohort','norm','link','ffn','advance_matrix','advance','advance_gather','continue')
for section in sections:
    run(str(cross)+'-objcopy','-O','binary','-j','.'+section,output/'program.elf',output/(section+'.bin'))
for tool,args,name in (('nm',['-n'],'program.symbols'),('nm',['-u'],'program.undefined'),('objdump',['-d'],'program.disassembly'),('objdump',['-h'],'program.sections')):
    with (output/name).open('w') as stream:
        subprocess.run([str(cross)+'-'+tool,*args,str(output/'program.elf')],stdout=stream,check=True)
assert not (output/'program.undefined').read_bytes()
symbols=(output/'program.symbols').read_text();disassembly=(output/'program.disassembly').read_text()
for name,address in (('cohort_entry',0x72000),('cohort_done',0x7200c),('cohort_park',0x72038),('cohort_fault_park',0x72060)):
    assert re.search('^'+format(address,'08x')+' T '+name+'$',symbols,re.M),name
for name in ('main','capture','once','load_recipe'):
    assert re.search(r'^[0-9a-f]+ T q38_resident_continue_'+name+'$',symbols,re.M),name
for section in sections:
    if section not in ('text','continue'):assert (output/(section+'.bin')).read_bytes()==(prior/(section+'.bin')).read_bytes(),section
terminal=(output/'text.bin').read_bytes()
assert len(terminal)==128 and terminal[0x38:0x40]==struct.pack('<II',0x0000006f,0xffdff06f) and terminal[0x40:0x60]==bytes(32)
assert terminal!=(prior/'text.bin').read_bytes()
assert not re.search(r'^[0-9a-f]+ [bBdDgGsS] ',symbols,re.M)
assert not re.search(r'\b(?:divu?|remu?|mulh[su]*|fdiv\.[sd]|fsqrt\.[sd]|fmadd|fmsub|fnmadd|fnmsub|fcvt\.wu?\.s|ebreak)\b',disassembly)
csr=[]
for line in disassembly.splitlines():
    match=re.match(r'\s*([0-9a-f]+):\s+([0-9a-f]{8})\s',line)
    if match and int(match[2],16)&127==0x73:
        address,word=int(match[1],16),int(match[2],16)
        assert (word>>12)&7==2 and (word>>15)&31==0 and (word>>7)&31 and word>>20 in (0xb00,0xb80)
        csr.append((address,word>>20))
assert len(csr)==9 and [v for _,v in csr]==[0xb80,0xb00,0xb80]*3
for group,(first,last) in enumerate(((0x40000,0x46000),(0x68000,0x6e000),(0x15c000,0x17e000))):
    a=csr[group*3:group*3+3];assert first<=a[0][0] and a[2][0]<last and a[1][0]==a[0][0]+4 and a[2][0]==a[0][0]+8
rows=(output/'program.sections').read_text().splitlines()
for i,row in enumerate(rows):
    match=re.match(r'\s*\d+\s+(\S+)\s+([0-9a-fA-F]+)\s+',row)
    if match and i+1<len(rows) and 'ALLOC' in rows[i+1]:assert match[1][1:] in sections,row
frames=base['base']['base']['frames'](output/'continue.su');assert frames and max(frames.values())<=2048,frames
for section in sections:
    data=(output/(section+'.bin')).read_bytes();print(hashlib.sha256(data).hexdigest()+' '+section+' bytes'+str(len(data)))
sys.path.insert(0,str(here/'tests'))
try:
    from pioneer_q38_resident_continue_image_common import verify
    verify(output)
finally:
    sys.path.pop(0)
print('PASS COMPILED continuation:actual layer/link/advance->new immutable device recipe admission+LIVE private sequence binding->actual complete following layer;maxNewFrame'+str(max(frames.values())))
print('OFFLINE ONLY;NEW native L1/GDDR journals/source/clock/host/init/physical qualification OWED;NOT full64/denseKV/state-token commit/FAST Work/inference-speed grant')
