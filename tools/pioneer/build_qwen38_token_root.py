#!/usr/bin/env python3
"""Experimental persistent parent BUILD/INSPECT only; never install or launch."""
import hashlib
import os
import pathlib
import re
import subprocess
import sys
options=sys.argv[3:]
assert 3<=len(sys.argv)<=6 and len(set(options))==len(options) and set(options)<={'--inspect-candidate','--cancel-boundary','--f32-hidden'}
inspect='--inspect-candidate' in options;cancel='--cancel-boundary' in options;f32='--f32-hidden' in options
assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
sdk,out=map(pathlib.Path,sys.argv[1:3]);here=pathlib.Path(__file__).resolve().parent
assert sdk.is_absolute() and out.is_absolute() and not out.exists() and not out.is_symlink()
tool=str(sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku-');assert pathlib.Path(tool+'g++').is_file()
out.mkdir(mode=0o700)
def run(*args):subprocess.run([str(a) for a in args],check=True)
# Reproduce the strict returning components and unchanged whole feedback
# image FIRST. Neither an old native owner nor a component alone can launch.
components=out/'qualified-components'
run(sys.executable,here/'build_qwen38_global_token_components.py',sdk,components,*(['--inspect-candidate'] if inspect else []),*(['--f32-hidden'] if f32 else []))
flags=['-std=c++11','-O2','-Wall','-Wextra','-Werror','-ffreestanding','-fno-exceptions','-fno-rtti','-fno-stack-protector',
 '-fno-pic','-fno-pie','-ffp-contract=off','-ffunction-sections','-fdata-sections','-fno-unwind-tables','-fno-asynchronous-unwind-tables',
 '-msmall-data-limit=0','-mno-relax','-march=rv32imf','-mabi=ilp32','-fstack-usage','-ffixed-s11']
run(tool+'g++',*flags,'-DPIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE=1','-DPIONEER_Q38_GLOBAL_CHAIN_FEEDBACK=1',
 *(['-DPIONEER_Q38_TOKEN_CANCEL_BOUNDARY=1'] if cancel else []),
 '-c',here/'blackhole_qwen38_token_root.cpp','-o',out/'program.o')
run(tool+'as','-march=rv32imf','-mabi=ilp32','-mno-relax',here/'blackhole_qwen38_global_chain_start.S','-o',out/'start.o')
run(tool+'ld','-m','elf32lriscv','--no-relax','--gc-sections','-T',here/'blackhole_qwen38_global_chain.ld',
 out/'start.o',out/'program.o',components/'token-components-NOBOOT.o',components/'unchanged-feedback/feedback.o',
 '-Map',out/'program.map','-o',out/'program.elf')
for section in ('boot','text','terminal','conv'):run(tool+'objcopy','-O','binary','-j','.'+section,out/'program.elf',out/(section+'.bin'))
symbols=subprocess.check_output([tool+'nm','-nC',str(out/'program.elf')],text=True)
assert not subprocess.check_output([tool+'nm','-u',str(out/'program.elf')],text=True).strip()
assert not re.search(r'^[0-9a-f]+ [bBdDgGsS] ',symbols,re.M)
for name,address in (('_start',0x6000),('scalar_entry',0x40000),('scalar_fault_park',0x60000),('q38_global_chain_stopped',0x60004)):
 assert re.search('^'+format(address,'08x')+' T '+name+'$',symbols,re.M),name
for name in ('q38_global_chain_main','q38_return_recurrent_layer','q38_return_dense_layer','q38_global_chain_feedback_decode'):
 assert re.search(r'^[0-9a-f]+ T '+name+'$',symbols,re.M),name
for name in ('GlobalChain::CommitState(', 'GlobalChain::PublishToken(', 'GlobalChain::Bootstrap(', 'DensePartitionRoot::AdmitGlobalOutput(', 'Joined::AppendTokenKv('):
 assert name in symbols,name
assert bool(re.search(r'^[0-9a-f]+ T q38_token_cancellation_point$',symbols,re.M))==cancel
for name in ('q38_layer_chain_main','q38_run_final_dense_and_stop','mixed_recurrent_block_main','mixed_live_block_main',
 'mixed_live_block_stopped','mixed_recurrent_block_stopped','mixed_recurrent_block_wait','mixed_live_block_wait','q38_feedback_unbound_embedding_main'):
 assert not re.search(r'\b'+name+r'\b',symbols),name
dis=subprocess.check_output([tool+'objdump','-d',str(out/'program.elf')],text=True)
# The linker intentionally puts read-only constants in .text too. Decode
# ONLY literal-ELF function intervals, not data bytes masquerading as opcodes.
intervals=[(0x6000,0x6004),(0x40000,0x40010),(0x60000,0x60008)]
for line in subprocess.check_output([tool+'objdump','-t',str(out/'program.elf')],text=True).splitlines():
 p=line.split()
 if len(p)==6 and p[2]=='F':
  start,size=int(p[0],16),int(p[4],16)
  if size:
   assert not (start|size)&3
   assert (0x40000<=start and start+size<=0x60000) or (0x64000<=start and start+size<=0x6c000)
   intervals.append((start,start+size))
instructions=[]
for line in dis.splitlines():
 m=re.match(r'\s*[0-9a-f]+:\s+([0-9a-f]{8})\s',line)
 if not m:continue
 address=int(line.split(':',1)[0].strip(),16)
 if not any(begin<=address and address+4<=end for begin,end in intervals):continue
 assert not address&3 and int(m[1],16)&3==3,'RV32IMF functions contain only 32-bit instructions'
 assert int(m[1],16)&127!=0x73,'no controller CSR'
 instructions.append(line)
code='\n'.join(instructions)
assert not re.search(r'\b(?:divu?|remu?|mulh[su]*|fdiv\.[sd]|fsqrt\.[sd]|fmadd|fmsub|fnmadd|fnmsub|fcvt\.wu?\.s|ebreak)\b',code)
assert len([s for s in instructions if re.search(r'\bmv\s+s11,\w+\s*$',s)])==5,'one parent + two existing bind/restore pairs'
frames=[int(line.split('\t')[1]) for line in (out/'program.su').read_text().splitlines()]
assert frames and max(frames)<=2048,max(frames)
sections=subprocess.check_output([tool+'objdump','-h',str(out/'program.elf')],text=True).splitlines()
for i,row in enumerate(sections):
 m=re.match(r'\s*\d+\s+(\S+)\s+([0-9a-fA-F]+)\s+',row)
 if m and 'ALLOC' in sections[i+1]:assert m[1] in ('.boot','.text','.terminal','.conv'),row
(out/'program.symbols').write_text(subprocess.check_output([tool+'nm','-n',str(out/'program.elf')],text=True))
(out/'program.disassembly').write_text(dis)
PINS=(('boot.bin',4,'32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca'),
 ('text.bin',128868,'4a110bff8be856fdf02011750bbb2ecf083c2d4a3e6e1c9decd92253018742c7'),
 ('terminal.bin',8,'6eb1d84addfac7202ad4f0d747c6e64d3b92c03025651c7dabe6c56c9cba95d5'),
 ('conv.bin',19752,'efb5924c156faafe3a66f7afbc70e253667cad25197d81f82f58789841a8d9bf'),
 ('program.elf',176516,'505eac1b08bee2a6841e5946252ef83d0b05def6bdcad9036b761bd08ea298ed'))
PINS_CANCEL=(('boot.bin',4,'32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca'),
 ('text.bin',129228,'e8d2ddb2b9df573bf63bd5739a8dc15321bbfc2be37ecdd43b89f8a6ce711462'),
 ('terminal.bin',8,'6eb1d84addfac7202ad4f0d747c6e64d3b92c03025651c7dabe6c56c9cba95d5'),
 ('conv.bin',21340,'1dd258b6602a98ff34e04c34c74a110de99a3b63c0168f580428422ffd128f15'),
 ('program.elf',178168,'4d78a5147336e32494e55459fd6b421ecd229f0518f95a2bef559ba346296b35'))
PINS_F32=(('boot.bin',4,'32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca'),
 ('text.bin',128876,'e59efb8ee42775eb3a49f0db9656c0fdb136eb52f81879c2416ba87dacca1900'),
 ('terminal.bin',8,'6eb1d84addfac7202ad4f0d747c6e64d3b92c03025651c7dabe6c56c9cba95d5'),
 ('conv.bin',19752,'3457413087afef8d24755f49546e0d968af87fb5fbad60b674924ae6850c6c51'),
 ('program.elf',176532,'9eb2a26fcca87c06345e1b5c63e36f91bf433d90b57d75513fed1e31439f2839'))
# Independently identical main+clean BUILD pins only; runtime qualification
# (including actual cancellation/combined stack) is separate and still owed.
PINS_CANCEL_F32=(('boot.bin',4,'32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca'),
 ('text.bin',129236,'20ace51c586a9f4ff3b8a2c21a2d26c21a01ee51a2ff4b22ffd9fb779713bd47'),
 ('terminal.bin',8,'6eb1d84addfac7202ad4f0d747c6e64d3b92c03025651c7dabe6c56c9cba95d5'),
 ('conv.bin',21340,'8d208e783507fbda17b7309d46c7e5a2b07f37874c6b600bacb375b69f0dd721'),
 ('program.elf',178180,'5d870cb7efcee1067ceaf146d05b916de80ca5b0f1be39f55a9303db3c018f70'))
observed=[]
for name in ('boot.bin','text.bin','terminal.bin','conv.bin','program.elf'):
 data=(out/name).read_bytes();item=(name,len(data),hashlib.sha256(data).hexdigest());observed.append(item);print(repr(item),flush=True)
expected=(PINS_CANCEL_F32 if cancel else PINS_F32) if f32 else PINS_CANCEL if cancel else PINS
if not inspect:assert expected and tuple(observed)==expected,'unqualified persistent parent variant; inspect independent builds first'
print('BUILD ONLY persistent parent/bootstrap/prompt+generation/all64+973/recurrent state copy+readback/token stream/final119STOP;maxFrame'+str(max(frames))+
 ';NO runtime/combined stack/original full-model/native/physical/cancellation/Work/Assistant/TPS qualification. DO NOT INSTALL')
