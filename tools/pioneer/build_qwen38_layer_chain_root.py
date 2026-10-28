#!/usr/bin/env python3
"""Build the separate all64/prefix4 on-card scheduler. NEVER install/run here."""
import hashlib
import os
import pathlib
import re
import subprocess
import sys
if len(sys.argv) not in (3,4) or len(sys.argv)==4 and sys.argv[3]!='--prefix4-test' or any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ):raise SystemExit(2)
prefix=len(sys.argv)==4;layers=4 if prefix else 64
sdk,out=map(pathlib.Path,sys.argv[1:3]);here=pathlib.Path(__file__).resolve().parent
assert sdk.is_absolute() and out.is_absolute() and not out.exists() and not out.is_symlink()
tool=str(sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku-');out.mkdir(mode=0o700)
def run(*args):subprocess.run([str(a) for a in args],check=True)
run(sys.executable,here/'build_qwen38_layer_return_components.py',sdk,out/'components','--chain-stop-prefix4' if prefix else '--chain-stop')
flags=['-std=c++11','-O2','-Wall','-Wextra','-Werror','-ffreestanding','-fno-exceptions','-fno-rtti','-fno-stack-protector',
 '-fno-pic','-fno-pie','-ffp-contract=off','-ffunction-sections','-fdata-sections','-fno-unwind-tables','-fno-asynchronous-unwind-tables',
 '-msmall-data-limit=0','-mno-relax','-march=rv32imf','-mabi=ilp32','-fstack-usage','-ffixed-s11',
 '-DPIONEER_Q38_LAYER_CHAIN_SCHEDULER_IMAGE=1','-DPIONEER_Q38_LAYER_CHAIN_LAYERS='+str(layers)]
if prefix:flags+=['-DPIONEER_Q38_LAYER_CHAIN_PREFIX4_TEST_IMAGE=1']
run(tool+'g++',*flags,'-c',here/'blackhole_qwen38_layer_chain_root.cpp','-o',out/'program.o')
run(tool+'as','-march=rv32imf','-mabi=ilp32','-mno-relax',here/'blackhole_qwen38_layer_chain_start.S','-o',out/'start.o')
run(tool+'ld','-m','elf32lriscv','--no-relax','--gc-sections','-T',here/'blackhole_qwen38_layer_chain.ld',
 out/'start.o',out/'program.o',out/'components/layer-components-NOBOOT.o','-Map',out/'program.map','-o',out/'program.elf')
for section in ('boot','text','terminal','conv'):run(tool+'objcopy','-O','binary','-j','.'+section,out/'program.elf',out/(section+'.bin'))
symbols=subprocess.check_output([tool+'nm','-n',str(out/'program.elf')],text=True)
assert not subprocess.check_output([tool+'nm','-u',str(out/'program.elf')],text=True).strip()
assert not re.search(r'^[0-9a-f]+ [bBdDgGsS] ',symbols,re.M)
for name,address in (('_start',0x6000),('scalar_entry',0x40000),('scalar_fault_park',0x60000),('q38_layer_chain_stopped',0x60004)):
 assert re.search('^'+format(address,'08x')+' T '+name+'$',symbols,re.M),name
for name in ('q38_layer_chain_main','q38_return_recurrent_layer','q38_run_final_dense_and_stop','q38_joined_dense_frame'):
 assert re.search(r'^[0-9a-f]+ T '+name+r'$',symbols,re.M),name
if not prefix:assert re.search(r'^[0-9a-f]+ T q38_return_dense_layer$',symbols,re.M)
for name in ('mixed_recurrent_block_main','mixed_live_block_main','mixed_live_block_stopped','mixed_recurrent_block_wait','mixed_live_block_wait','q38_mixed_root_seal'):
 assert not re.search(r'\b'+name+r'\b',symbols),name
dis=subprocess.check_output([tool+'objdump','-d',str(out/'program.elf')],text=True)
assert not re.search(r'\b(?:divu?|remu?|mulh[su]*|fdiv\.[sd]|fsqrt\.[sd]|fmadd|fmsub|fnmadd|fnmsub|fcvt\.wu?\.s|ebreak)\b',dis)
for line in dis.splitlines():
 m=re.match(r'\s*[0-9a-f]+:\s+([0-9a-f]{8})\s',line)
 if m:assert int(m[1],16)&127!=0x73,'no controller CSR'
writes=[line for line in dis.splitlines() if re.search(r'\bmv\s+s11,\w+\s*$',line)]
assert len(writes) in ((4,6) if prefix else (6,)),len(writes)
frames=[int(line.split('\t')[1]) for line in (out/'program.su').read_text().splitlines()];assert frames and max(frames)<=2048
sections=subprocess.check_output([tool+'objdump','-h',str(out/'program.elf')],text=True).splitlines()
for i,row in enumerate(sections):
 m=re.match(r'\s*\d+\s+(\S+)\s+([0-9a-fA-F]+)\s+',row)
 if m and 'ALLOC' in sections[i+1]:assert m[1] in ('.boot','.text','.terminal','.conv'),row
(out/'program.symbols').write_text(symbols);(out/'program.disassembly').write_text(dis)
PINS64=(('boot.bin',4,'32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca'),
 ('text.bin',103620,'625fb65e3b9f0a340033ef941a22ebf0df7081544236d7246277b4b9ac46db1f'),
 ('terminal.bin',8,'6eb1d84addfac7202ad4f0d747c6e64d3b92c03025651c7dabe6c56c9cba95d5'),
 ('conv.bin',7500,'96cbee95485ba5bd768abc6d27c5c9207fd26a5004c2563778d88f64353b787e'),
 ('program.elf',136572,'95f7a714f05f84a15f56835cc620bdeb3999540975f675b0c2f9f54f96a1e9ff'))
PINS4=(('boot.bin',4,'32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca'),
 ('text.bin',103056,'815ec2fa11434f4407ae72d139532d366e23514f1a61ba45a35e170bffc588b1'),
 ('terminal.bin',8,'6eb1d84addfac7202ad4f0d747c6e64d3b92c03025651c7dabe6c56c9cba95d5'),
 ('conv.bin',7500,'9ffb41efc2d89443d0e1d4d086bb343ac019b98e8ede997ca0099855e596eab9'),
 ('program.elf',136516,'2963e12cdc3d64bac2c4f5bbd368964fcfd635696c70a90261fb5d3bf5fb251a'))
for name,size,digest in (PINS4 if prefix else PINS64):
 data=(out/name).read_bytes();actual=hashlib.sha256(data).hexdigest();assert len(data)==size and actual==digest,name
 print(actual+' '+name+' bytes'+str(len(data)))
print('PINNED BUILD ONLY chain'+str(layers)+' SAME on-card scheduler/actual returning recurrent+dense+final dense STOP bodies/maxROOTframe'+str(max(frames))+
 ';NO token output/head/sampling/state commit/native/physical/Work/TPS qualification. Connected runtime/combined stack/STOP STILL OWED. DO NOT INSTALL')
