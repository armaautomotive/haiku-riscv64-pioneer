#!/usr/bin/env python3
"""BUILD ONLY: original full-EOG ROOT with generation-only phase timing.

Requires an independently pinned full-batched control; relinks it and
reproduces all three original TUs before building the changed ROOT. This
does not select a native owner, worker, learned payload or physical trial.
"""
import hashlib
import os
from pathlib import Path
import re
import subprocess
import sys
from generate_qwen38_token_programs import authenticated, pins

PINS=(
 ('boot.bin',4,'32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca'),
 ('text.bin',130580,'0b118528adadb2ab5be46c8d263ebbc9ff3dab02488efea9c9e3b437a6125b34'),
 ('terminal.bin',8,'468eb84a74c067cc328b3fb3e51b768a8440cd67d292add55eef6861c9b24de2'),
 ('conv.bin',32216,'b76f012be20af78ae5d7a3b027bf22e8dbab56ae2f7f0182f669832c2fc4de59'),
 ('fast_decode.bin',1496,'a37bab87fc21f140236413c673352966bf9452212bf89e7956868c1cea15f9d1'),
 ('program.elf',193672,'882d2e1cf08ad5ccbd69c52a7ecb7daf5e7101527f669431faf962f158b5b425'),
) # Whole BUILD identity only; independent main/clean execution still required.
args=sys.argv[1:];inspect='--inspect-candidate' in args
if inspect:
 assert args.count('--inspect-candidate')==1;args.remove('--inspect-candidate')
assert __debug__ and len(args)==3
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
sdk,control,out=map(Path,args);here=Path(__file__).resolve().parent
assert all(p.is_absolute() for p in (sdk,control,out))
assert control.is_dir() and not control.is_symlink() and not out.exists() and not out.is_symlink()
out.mkdir(mode=0o700)
old=authenticated(control,pins(here/'build_qwen38_full_offload_batched_root.py','PINS'))
tool=str(sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku-');commands=[]
def run(*values):
 command=list(map(str,values));commands.append(command);subprocess.run(command,check=True)
def rebase(command):
 command=list(command)
 assert command[0]==tool+'g++' and command.count('-c')==command.count('-o')==1
 command[command.index('-c')+1]=str(here/Path(command[command.index('-c')+1]).name)
 for at,value in enumerate(command[:-1]):
  if value=='-include':command[at+1]=str(here/Path(command[at+1]).name)
 return command
base=control/'unchanged-output-closure'
saved=(control/'build-argv.txt').read_text().splitlines()
link=saved[1].split('\t');assert link[0]==tool+'ld' and link.count('-T')==1
entry=Path(link[link.index('-T')+2]);feedback=next(Path(p) for p in link if Path(p).name=='feedback.o')
conv=feedback.with_name('root_conv_sections.o')
run(tool+'ld','-m','elf32lriscv','--no-relax','--gc-sections','-T',here/'blackhole_qwen38_global_chain.ld',
 entry,control/'token.o',base/'components-NOBOOT.o',feedback,'-o',out/'entire-control.elf')
assert (out/'entire-control.elf').read_bytes()==old['program.elf'],'ENTIRE accepted original ROOT FIRST'
objects=[];compile_commands={}
for kind,original in (('recurrent',base/'output-closure-recurrent.o'),('dense',base/'output-closure-dense.o'),('token',control/'token.o')):
 command=rebase(saved[0].split('\t') if kind=='token' else (base/('compile-'+kind+'-argv.txt')).read_text().splitlines())
 unchanged=out/('unchanged-'+kind+'.o');command[command.index('-o')+1]=str(unchanged)
 run(*command);assert unchanged.read_bytes()==original.read_bytes(),'ENTIRE old '+kind+' TU FIRST'
 target=out/('fast-'+kind+'.o');command[command.index('-o')+1]=str(target)
 command[1:1]=['-DPIONEER_Q38_FAST_DECODE_PROFILE_IMAGE=1']
 if kind=='token':command[1:1]=['-DPIONEER_Q38_TOKEN_PHASE_PROFILE_IMAGE=1']
 command[1:1]=['-MMD','-MF',str(out/(kind+'.d'))]
 run(*command);objects.append(target);compile_commands[kind]=command[:]
 (out/('compile-'+kind+'-argv.txt')).write_text('\n'.join(command)+'\n')
helper=out/'fast-helper.o';command=compile_commands['token'][:]
command[command.index('-c')+1]=str(here/'blackhole_qwen38_fast_decode_profile.cpp')
command[command.index('-o')+1]=str(helper);command[command.index('-MF')+1]=str(out/'helper.d')
command[command.index('-O2')]='-Os';run(*command)
(out/'compile-helper-argv.txt').write_text('\n'.join(command)+'\n')
raw=out/'components-raw.o';capsule=out/'components-NOBOOT.o'
run(tool+'ld','-m','elf32lriscv','--no-relax','-r','--gc-sections',
 '-u','q38_return_recurrent_layer','-u','q38_return_dense_layer','-u','q38_global_chain_learned_rms',
 '-u','q38_global_chain_quantize14',objects[0],objects[1],conv,'-o',raw)
dead=('mixed_recurrent_block_wait','mixed_recurrent_block_stopped','mixed_live_block_wait','mixed_live_block_stopped')
relocations=subprocess.check_output([tool+'objdump','-r',str(raw)],text=True)
for name in dead:assert not re.search(r'\b'+name+r'\b',relocations)
run(tool+'objcopy',*['--strip-symbol='+name for name in dead],raw,capsule)
unresolved=subprocess.check_output([tool+'nm','-u',str(capsule)],text=True).splitlines()
assert {row.split()[-1] for row in unresolved}=={'scalar_fault_park','q38_fast_decode_profile_enter','q38_token_phase_mark'}
layout=(here/'blackhole_qwen38_global_chain.ld').read_text();anchor=' .text 0x40000'
assert layout.count(anchor)==1
layout=layout.replace(anchor,' .fast_decode 0x6c000 : { *(.fast.decode.profile .text.*PioneerQ38FastDecodeProfileData*) . = ALIGN(4); }\n'+anchor)
layout=layout.replace(' ASSERT(SIZEOF(.forbidden)==0',
 ' ASSERT(SIZEOF(.fast_decode)>0 && SIZEOF(.fast_decode)<=0x2000,"distinct timing helper;NEW native code ownership owed")\n ASSERT(SIZEOF(.forbidden)==0')
(out/'program.ld').write_text(layout)
elf=out/'program.elf'
run(tool+'ld','-m','elf32lriscv','--no-relax','--gc-sections','-T',out/'program.ld',entry,objects[2],helper,capsule,feedback,
 '-Map',out/'program.map','-o',elf)
assert not subprocess.check_output([tool+'nm','-u',str(elf)]).strip()
symbols=subprocess.check_output([tool+'nm','-n',str(elf)],text=True)
assert not re.search(r'^[0-9a-f]+ [bBdDgGsS] ',symbols,re.M)
for name,address in (('_start',0x6000),('scalar_entry',0x40000),('scalar_fault_park',0x60000),('q38_global_chain_stopped',0x60004)):
 assert re.search('^'+format(address,'08x')+' T '+name+'$',symbols,re.M),name
functions=[]
for row in map(str.split,subprocess.check_output([tool+'objdump','-t',str(elf)],text=True).splitlines()):
 if len(row)==6 and row[2]=='F' and int(row[4],16):
  first,size=int(row[0],16),int(row[4],16);assert not (first|size)&3
  assert 0x40000<=first<first+size<=0x60000 or 0x64000<=first<first+size<=0x6e000
  functions.append((first,first+size,row[5]))
dis=subprocess.check_output([tool+'objdump','-d',str(elf)],text=True);instructions=[];csr=[];calls={}
for match in re.finditer(r'^\s*([0-9a-f]+):\s+([0-9a-f]{8})\s.*$',dis,re.M):
 owners={name for first,last,name in functions if first<=int(match[1],16)<last}
 if not owners:continue
 word=int(match[2],16);assert word&3==3 and not(word&127==0x67 and not((word>>15)&31))
 if word&127==0x73:
  assert owners=={'q38_token_phase_clock'} and (word>>12)&7==2 and not((word>>15)&31) and (word>>7)&31
  csr.append(word>>20)
 call=re.search(r'\bjalr\b.*# [0-9a-f]+ <(q38_token_phase_[a-z]+|q38_fast_decode_profile_[a-z]+)>',match[0])
 if call:
  assert len(owners)==1
  key=(next(iter(owners)),call[1]);calls[key]=calls.get(key,0)+1
 instructions.append(match[0])
assert csr==[0xb80,0xb00,0xb80]
assert calls=={
 ('q38_global_chain_main','q38_token_phase_begin'):1,
 ('q38_global_chain_main','q38_fast_decode_profile_position'):1,
 ('q38_global_chain_main','q38_token_phase_mark'):3,
 ('_ZN10ProjectionL3RunEPKj','q38_fast_decode_profile_enter'):2,
 ('_ZN10ProjectionL3RunEPKj','q38_token_phase_mark'):8,
 ('_ZN11GlobalChainL6FinishEPKjj','q38_token_phase_mark'):1,
 ('_ZN11GlobalChainL6FinishEPKjj','q38_token_phase_complete'):1,
 ('_ZN11GlobalChainL7RunHeadEv','q38_token_phase_mark'):9,
 ('q38_fast_decode_profile_position','q38_token_phase_clock'):1,
 ('q38_token_phase_mark','q38_token_phase_clock'):1,
 ('q38_token_phase_complete','q38_token_phase_clock'):1,
 ('q38_fast_decode_profile_enter','q38_token_phase_mark'):1,
},calls # Emitted call sites ONLY, not full model/owner qualification.
assert not re.search(r'\b(?:divu?|remu?|mulh[su]*|fdiv\.[sd]|fsqrt\.[sd]|fmadd|fmsub|fnmadd|fnmsub|fcvt\.wu?\.s|ebreak)\b','\n'.join(instructions))
for obj in (*objects,helper):
 frames=[row.split('\t') for row in obj.with_suffix('.su').read_text().splitlines()]
 assert frames and all(len(r)==3 and r[2]=='static' and int(r[1])<=4096 for r in frames)
sections={'boot':4,'text':131072,'terminal':8,'conv':32768,'fast_decode':8192}
table=subprocess.check_output([tool+'objdump','-h',str(elf)],text=True).splitlines()
for i,row in enumerate(table):
 match=re.match(r'\s*\d+\s+(\S+)\s+([0-9a-f]+)\s+',row)
 if match and 'ALLOC' in table[i+1]:assert match[1][1:] in sections,row
for section,limit in sections.items():
 run(tool+'objcopy','-O','binary','-j','.'+section,elf,out/(section+'.bin'))
 data=(out/(section+'.bin')).read_bytes();assert data and not len(data)%4 and len(data)<=limit
 if section in ('boot','terminal'):assert data==old[section+'.bin']
 print(section,len(data),hashlib.sha256(data).hexdigest(),flush=True)
print('program.elf',elf.stat().st_size,hashlib.sha256(elf.read_bytes()).hexdigest(),flush=True)
if not inspect:
 for name,size,digest in PINS:
  data=(out/name).read_bytes();assert len(data)==size and hashlib.sha256(data).hexdigest()==digest,name
(out/'program.symbols').write_text(symbols);(out/'program.disassembly').write_text(dis)
(out/'build-argv.txt').write_text('\n'.join('\t'.join(c) for c in commands)+'\n')
print('INSPECT_BUILD_ONLY original full-EOG ROOT FIRST;generation-only v3 timing/new6c000 code view;'
 'NO worker arithmetic change,native owner,card execution,Assistant,model quality or speed grant',flush=True)
