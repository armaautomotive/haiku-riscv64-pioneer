#!/usr/bin/env python3
"""Build the experimental per-layer history-staging ROOT. No install/card IO."""
import hashlib
import os
import pathlib
import re
import subprocess
import sys
if len(sys.argv)!=3 or any(k.startswith('PIONEER_') for k in os.environ):raise SystemExit(2)
sdk,out=map(pathlib.Path,sys.argv[1:]);here=pathlib.Path(__file__).resolve().parent
assert sdk.is_absolute() and out.is_absolute() and not out.exists() and not out.is_symlink()
cross=str(sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku');assert pathlib.Path(cross+'-g++').is_file()
out.mkdir(mode=0o700)
def run(*args):subprocess.run(list(map(str,args)),check=True)
flags=['-std=c++11','-O2','-Wall','-Wextra','-Werror','-ffreestanding','-fno-exceptions','-fno-rtti','-fno-stack-protector',
 '-fno-pic','-fno-pie','-ffp-contract=off','-ffunction-sections','-fdata-sections','-fno-unwind-tables','-fno-asynchronous-unwind-tables',
 '-msmall-data-limit=0','-mno-relax','-march=rv32imf','-mabi=ilp32','-fstack-usage','-ffixed-s11',
 '-DPIONEER_Q38_MIXED_RECURRENT_BLOCK_ROOT_IMAGE=1','-DPIONEER_Q38_SIGMOID_PRECISION_IMAGE=1',
 '-DPIONEER_Q38_RECURRENT_HISTORY_STAGING_IMAGE=1']
run(cross+'-g++',*flags,'-c',here/'blackhole_qwen38_mixed_recurrent_block_root.cpp','-o',out/'program.o')
run(cross+'-g++',*flags,'-DPIONEER_Q38_MIXED_ROOT_CAPSULE_IMAGE=1','-Dscalar_main=q38_mixed_recurrent_root_conv',
 '-include',here/'PioneerQ38MixedRecurrentBlockRootImageRole.h','-c',here/'blackhole_qwen38_conv_batch_silu_precision.cpp','-o',out/'root_conv.o')
run(cross+'-as','-march=rv32imf','-mabi=ilp32','-mno-relax',here/'blackhole_qwen38_mixed_recurrent_block_root_start.S','-o',out/'start.o')
run(cross+'-ld','-m','elf32lriscv','--no-relax','--gc-sections','-T',here/'blackhole_qwen38_mixed_recurrent_block_root.ld',
 out/'start.o',out/'program.o',out/'root_conv.o','-Map',out/'program.map','-o',out/'program.elf')
for section in ('boot','text','conv'):run(cross+'-objcopy','-O','binary','-j','.'+section,out/'program.elf',out/(section+'.bin'))
for tool,args,name in (('nm',['-n'],'symbols'),('nm',['-u'],'undefined'),('objdump',['-d'],'disassembly'),('objdump',['-h'],'sections')):
 with (out/('program.'+name)).open('w') as target:subprocess.run([cross+'-'+tool,*args,str(out/'program.elf')],stdout=target,check=True)
assert not (out/'program.undefined').read_bytes()
symbols=(out/'program.symbols').read_text();dis=(out/'program.disassembly').read_text()
assert not re.search(r'^[0-9a-f]+ [bBdDgGsS] ',symbols,re.M)
assert all(s not in symbols for s in ('mixed_projection_main','mixed_recurrent_cohort_main','q38_unused_dense_frame'))
assert not re.search(r'\b(?:divu?|remu?|mulh[su]*|fdiv\.[sd]|fsqrt\.[sd]|fmadd|fmsub|fnmadd|fnmsub|fcvt\.wu?\.s|ebreak)\b',dis)
for line in dis.splitlines():
 m=re.match(r'\s*[0-9a-f]+:\s+([0-9a-f]{8})\s',line)
 if m:assert int(m[1],16)&127!=0x73
reserved=[s for s in dis.splitlines() if re.search(r'\bs11\b',s)]
assert len(reserved)==2 and sum(bool(re.search(r'\bmv\s+s11,\w+\s*$',s)) for s in reserved)==1
assert sum(bool(re.search(r'\bmv\s+\w+,s11\s*$',s)) for s in reserved)==1
for name,address in (('scalar_entry',0x40000),('mixed_recurrent_block_wait_load',0x4004c),('mixed_recurrent_block_wait_branch',0x40050),
 ('mixed_recurrent_block_stopped',0x40060),('scalar_fault_park',0x40080),('mixed_recurrent_block_fault_stopped',0x4009c)):
 assert re.search('^'+format(address,'08x')+' T '+name+'$',symbols,re.M),name
frames=[int(s.split('\t')[1]) for name in ('program','root_conv') for s in (out/(name+'.su')).read_text().splitlines()]
assert frames and max(frames)<=4096
rows=(out/'program.sections').read_text().splitlines()
for i,row in enumerate(rows):
 m=re.match(r'\s*\d+\s+(\S+)\s+([0-9a-fA-F]+)\s+',row)
 if m and 'ALLOC' in rows[i+1]:assert m[1] in ('.boot','.text','.conv'),row
PINS=(('boot.bin',4,'32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca'),
 ('text.bin',53084,'6ad600096827d4f60b4cc73b7901a6999bd5c75c5311b32e2763fe700e6b7652'),
 ('conv.bin',7500,'16ff564dc406a72a918f7c67c40f6815a7430f6d02161a0be0b26ec56cde7f65'),
 ('program.elf',75184,'a1ae94663803c6cfc760ef242275658008cda99b5611e193a11a9958a3375339'))
for name,size,digest in PINS:
 data=(out/name).read_bytes();assert len(data)==size and hashlib.sha256(data).hexdigest()==digest,name
 print(digest+' '+name+' bytes'+str(len(data)))
print('BUILT_PINNED history-staging experimental ROOT/maxFrame'+str(max(frames))+
 ';NOT runtime qualified/native/current-history or token commit/fullmodel/Work/TPS;NEVER install before separate native qualification')
