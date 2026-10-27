#!/usr/bin/env python3
"""QBL1 device-input/fresh-admission/STOP lifecycle; no device IO."""
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
run(sys.executable,here/'build_qwen38_mixed_block_root_image.py',sdk,out/'unchanged-block-root')
for name,size,digest in (
 ('boot.bin',4,'32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca'),
 ('code.bin',44664,'61aada5043559c58827b205f8a0cb637822376e445b366d1217b783418443e4b'),
 ('program.elf',56876,'51498d12a94d782e817a42d92b0f1cc76bb35cec91aaea367481e8655066e311')):
 data=(out/'unchanged-block-root'/name).read_bytes();assert len(data)==size and hashlib.sha256(data).hexdigest()==digest,name
run(str(cross)+'-g++','-std=c++11','-O2','-Wall','-Wextra','-Werror','-ffreestanding','-fno-exceptions','-fno-rtti',
 '-fno-stack-protector','-fno-pic','-fno-pie','-ffp-contract=off','-ffunction-sections','-fdata-sections','-fno-unwind-tables',
 '-fno-asynchronous-unwind-tables','-msmall-data-limit=0','-mno-relax','-march=rv32imf','-mabi=ilp32','-fstack-usage','-ffixed-s11',
 '-DPIONEER_Q38_MIXED_LIVE_BLOCK_ROOT_IMAGE=1','-c',here/'blackhole_qwen38_mixed_live_block_root.cpp','-o',out/'program.o')
run(str(cross)+'-as','-march=rv32imf','-mabi=ilp32','-mno-relax',here/'blackhole_qwen38_mixed_live_block_root_start.S','-o',out/'start.o')
link=(here/'blackhole_qwen38_dense_partition_root.ld').read_text()
assert link.count('.text 0x50000')==1 and link.count('scalar_entry==0x50000')==1
link=link.replace('.text 0x50000','.text 0x40000').replace('scalar_entry==0x50000','scalar_entry==0x40000').replace('<=0x8000','<=0x20000')
(out/'program.ld').write_text(link)
run(str(cross)+'-ld','-m','elf32lriscv','--no-relax','--gc-sections','-T',out/'program.ld',out/'start.o',out/'program.o','-Map',out/'program.map','-o',out/'program.elf')
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
for name,address in (('scalar_entry',0x40000),('mixed_live_block_wait_load',0x4004c),('mixed_live_block_wait_branch',0x40050),('mixed_live_block_stopped',0x40060),('scalar_fault_park',0x40080),('mixed_live_block_fault_stopped',0x4009c)):
 assert re.search('^'+format(address,'08x')+' T '+name+'$',symbols,re.M),name
frames=[int(s.split('\t')[1]) for s in (out/'program.su').read_text().splitlines()];assert frames and max(frames)<=4096
rows=(out/'program.sections').read_text().splitlines()
for i,row in enumerate(rows):
 m=re.match(r'\s*\d+\s+(\S+)\s+([0-9a-fA-F]+)\s+',row)
 if m and 'ALLOC' in rows[i+1]:assert m[1] in ('.boot','.text'),row
PINS=(
 ('boot.bin',4,'32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca'),
 ('code.bin',50624,'8a05f3fa62bf8674f23783f3cb9aa0ffba7c564fc03b5b4b56ee267cc2fc06bc'),
 ('program.elf',62908,'2a90609c9033331430abcf054f637b65134d43bbf9fbf41672a194c5fd5a3bbc'))
for name,size,digest in PINS:
 data=(out/name).read_bytes();assert len(data)==size and hashlib.sha256(data).hexdigest()==digest,name
 print(digest+' '+name+' bytes'+str(size))
print('PASS NEW QBL1/device-input/verified virgin loops/persistent complete dense block/real119 STOP+ROOT terminal/maxFrame'+str(max(frames))+';native ownership+journals/full64/model/speed STILL OWED')
