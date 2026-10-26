#!/usr/bin/env python3
"""NEW QJR1 joined on-card controller; no native or device IO."""
import hashlib
import os
import pathlib
import re
import subprocess
import sys
if len(sys.argv)!=3 or any(k.startswith('PIONEER_') for k in os.environ):
    raise SystemExit(2)
sdk,out=map(pathlib.Path,sys.argv[1:])
assert sdk.is_absolute() and out.is_absolute() and not out.exists() and not out.is_symlink()
here=pathlib.Path(__file__).resolve().parent;cross=sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku'
assert pathlib.Path(str(cross)+'-g++').is_file();out.mkdir(mode=0o700)
def run(*args):subprocess.run([str(a) for a in args],check=True)
run(sys.executable,here/'build_qwen38_mixed_projection_root_image.py',sdk,out/'unchanged-projection-root')
for name,size,digest in (
 ('boot.bin',4,'ce5f73f31ecb80edc4dd267e1f46b0534a0201b837ba6d6231a5d4271df10102'),
 ('code.bin',16580,'f84c41dd22582d170086d2cac98286d0d8e208d2e7c7ee58ce828436b57a0192'),
 ('program.elf',26840,'e1b9bbc183a31b503b6b9ce89cb4f7329901cb335fe267b6d2aa5732027411b7')):
 data=(out/'unchanged-projection-root'/name).read_bytes();assert len(data)==size and hashlib.sha256(data).hexdigest()==digest,name
run(str(cross)+'-g++','-std=c++11','-O2','-Wall','-Wextra','-Werror','-ffreestanding','-fno-exceptions','-fno-rtti',
 '-fno-stack-protector','-fno-pic','-fno-pie','-ffp-contract=off','-ffunction-sections','-fdata-sections','-fno-unwind-tables',
 '-fno-asynchronous-unwind-tables','-msmall-data-limit=0','-mno-relax','-march=rv32imf','-mabi=ilp32','-fstack-usage','-ffixed-s11',
 '-DPIONEER_Q38_MIXED_JOINED_ROOT_IMAGE=1','-c',here/'blackhole_qwen38_mixed_joined_root.cpp','-o',out/'program.o')
run(str(cross)+'-as','-march=rv32imf','-mabi=ilp32','-mno-relax',here/'blackhole_qwen38_mixed_joined_root_start.S','-o',out/'start.o')
link=(here/'blackhole_qwen38_dense_partition_root.ld').read_text()
assert link.count('.text 0x50000')==1 and link.count('scalar_entry==0x50000')==1
link=link.replace('.text 0x50000','.text 0x40000').replace('scalar_entry==0x50000','scalar_entry==0x40000').replace('<=0x8000','<=0x20000')
(out/'program.ld').write_text(link)
run(str(cross)+'-ld','-m','elf32lriscv','--no-relax','--gc-sections','-T',out/'program.ld',out/'start.o',out/'program.o',
 '-Map',out/'program.map','-o',out/'program.elf')
for section,name in (('text','code'),('boot','boot')):run(str(cross)+'-objcopy','-O','binary','-j','.'+section,out/'program.elf',out/(name+'.bin'))
for tool,args,name in (('nm',['-n'],'symbols'),('nm',['-u'],'undefined'),('objdump',['-d'],'disassembly'),('objdump',['-h'],'sections')):
 with (out/('program.'+name)).open('w') as target:subprocess.run([str(cross)+'-'+tool,*args,str(out/'program.elf')],stdout=target,check=True)
assert not (out/'program.undefined').read_bytes()
symbols=(out/'program.symbols').read_text();dis=(out/'program.disassembly').read_text()
assert 'q38_joined_dense_frame' in symbols and not re.search(r'^[0-9a-f]+ [bBdDgGsS] ',symbols,re.M)
assert not re.search(r'\b(?:divu?|remu?|mulh[su]*|fdiv\.[sd]|fsqrt\.[sd]|fmadd|fmsub|fnmadd|fnmsub|fcvt\.wu?\.s|ebreak)\b',dis)
for line in dis.splitlines():
 m=re.match(r'\s*[0-9a-f]+:\s+([0-9a-f]{8})\s',line)
 if m:assert int(m[1],16)&127!=0x73,'no controller CSR'
reserved=[s for s in dis.splitlines() if re.search(r'\bs11\b',s)]
assert len(reserved)==2 and sum(bool(re.search(r'\bmv\s+s11,\w+\s*$',s)) for s in reserved)==1
assert sum(bool(re.search(r'\bmv\s+\w+,s11\s*$',s)) for s in reserved)==1
for name,address in (('scalar_entry',0x40000),('mixed_joined_wait_load',0x4004c),('mixed_joined_wait_branch',0x40050),('scalar_fault_park',0x40080),('mixed_joined_fault_stopped',0x4009c)):
 assert re.search('^'+format(address,'08x')+' T '+name+'$',symbols,re.M),name
frames=[int(s.split('\t')[1]) for s in (out/'program.su').read_text().splitlines()]
assert frames and max(frames)<=4096
rows=(out/'program.sections').read_text().splitlines()
for i,row in enumerate(rows):
 m=re.match(r'\s*\d+\s+(\S+)\s+([0-9a-fA-F]+)\s+',row)
 if m and 'ALLOC' in rows[i+1]:assert m[1] in ('.boot','.text'),row
for name in ('boot.bin','code.bin','program.elf'):
 data=(out/name).read_bytes();print(hashlib.sha256(data).hexdigest()+' '+name+' bytes'+str(len(data)))
for name,size,digest in (
 ('boot.bin',4,'32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca'),
 ('code.bin',36160,'27c5a3685683db74d8b18d1112aa06f82f9599cae2b03e4b550c3b57615108e5'),
 ('program.elf',47736,'9dc2716e040a0430e456112feca01d452612edbca02a77e4eec5503ae7dc0b69')):
 data=(out/name).read_bytes();assert len(data)==size and hashlib.sha256(data).hexdigest()==digest,name
print('PASS NEW QJR1/single persistent ROOT/QG-K-V -> four on-card prepare groups -> retained attention -> real6144 O input/no peer reset or STOP/maxFrame'+str(max(frames))+';NO native/full64/speed grant')
