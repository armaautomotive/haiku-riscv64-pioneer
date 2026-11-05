#!/usr/bin/env python3
"""Whole prepared ROOT candidate BUILD;new code view has NO native grant."""
import hashlib,os,pathlib,re,runpy,subprocess,sys
if not __debug__ or os.environ.get('PYTHONOPTIMIZE'):raise SystemExit('Refuse optimized Python;whole ROOT checks are required')
args=sys.argv[1:];assert args.count('--inspect-candidate')==1;args.remove('--inspect-candidate')
counter_unroll='--counter-unroll' in args
if counter_unroll:
 assert args.count('--counter-unroll')==1;args.remove('--counter-unroll')
phase_profile='--phase-profile' in args
if phase_profile:
 assert counter_unroll and args.count('--phase-profile')==1;args.remove('--phase-profile')
reuse=None
if '--control' in args:
 assert args.count('--control')==1;at=args.index('--control');reuse=pathlib.Path(args[at+1]);del args[at:at+2]
 assert reuse.is_absolute() and reuse.is_dir() and not reuse.is_symlink()
assert len(args)==2
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
sdk,out=map(pathlib.Path,args);here=pathlib.Path(__file__).resolve().parent
assert sdk.is_absolute() and out.is_absolute() and not out.exists() and not out.is_symlink()
out.mkdir(mode=0o700);saved=sys.argv[:]
if reuse is None:
 try:
  sys.argv=[str(here/'build_qwen38_full_offload_batched_root.py'),str(sdk),str(out/'unchanged-full')]
  runpy.run_path(sys.argv[0],run_name='__main__')
 finally:sys.argv=saved
control=reuse if reuse is not None else out/'unchanged-full'
assert hashlib.sha256((control/'program.elf').read_bytes()).hexdigest()=='e93c1a57c21de073233689741e00ddc9e55ad20b1fd74d0758aa3100ce313f66'
tool=str(sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku-');commands=[]
def run(*values):
 command=list(map(str,values));commands.append(command);subprocess.run(command,check=True)
base=control/'unchanged-output-closure'
link=(control/'build-argv.txt').read_text().splitlines()[1].split('\t')
assert link[0]==tool+'ld' and link.count('-T')==1
entry=pathlib.Path(link[link.index('-T')+2]);feedback=next(pathlib.Path(p) for p in link if pathlib.Path(p).name=='feedback.o')
conv=feedback.with_name('root_conv_sections.o');old_capsule=base/'components-NOBOOT.o'
# Reused controls are not a presence/lock-file shortcut:relink their ENTIRE
# old ROOT with the current unchanged LD and check its whole accepted hash.
run(tool+'ld','-m','elf32lriscv','--no-relax','--gc-sections','-T',here/'blackhole_qwen38_global_chain.ld',entry,control/'token.o',old_capsule,feedback,'-o',out/'entire-old-root.elf')
assert (out/'entire-old-root.elf').read_bytes()==(control/'program.elf').read_bytes()
objects=[]
for kind,original in zip(('recurrent','dense','token'),(base/'output-closure-recurrent.o',base/'output-closure-dense.o',control/'token.o')):
 if kind=='token':command=(control/'build-argv.txt').read_text().splitlines()[0].split('\t')
 else:command=(control/'unchanged-output-closure'/('compile-'+kind+'-argv.txt')).read_text().splitlines()
 assert command[0]==tool+'g++' and command.count('-c')==command.count('-o')==1
 command[command.index('-c')+1]=str(here/pathlib.Path(command[command.index('-c')+1]).name)
 old=out/('unchanged-'+kind+'.o');command[command.index('-o')+1]=str(old);run(*command)
 assert old.read_bytes()==original.read_bytes(),'ENTIRE old ROOT TU FIRST'
 target=out/('prepared-'+kind+'.o');command[command.index('-o')+1]=str(target)
 command=[command[0],'-DPIONEER_Q38_PREPARED_Q4_ROOT_IMAGE=1','-include',str(here/'PioneerQ38PreparedQ4RootRole.h'),*command[1:]]
 if counter_unroll:command[1:1]=['-DPIONEER_Q38_ROOT_COUNTER_UNROLL_IMAGE=1','-include',str(here/'PioneerQ38RootCounterUnrollRole.h')]
 run(*command);objects.append(target);(out/('compile-'+kind+'-argv.txt')).write_text('\n'.join(command)+'\n')
 frames=[row.split('\t') for row in target.with_suffix('.su').read_text().splitlines()]
 assert frames and all(len(row)==3 and row[2]=='static' and int(row[1])<=4096 for row in frames)
raw=out/'components-raw.o';capsule=out/'components-NOBOOT.o'
run(tool+'ld','-m','elf32lriscv','--no-relax','-r','--gc-sections','-u','q38_return_recurrent_layer','-u','q38_return_dense_layer',
 '-u','q38_global_chain_learned_rms','-u','q38_global_chain_quantize14',objects[0],objects[1],conv,'-o',raw)
dead=('mixed_recurrent_block_wait','mixed_recurrent_block_stopped','mixed_live_block_wait','mixed_live_block_stopped')
relocations=subprocess.check_output([tool+'objdump','-r',str(raw)],text=True)
for name in dead:assert not re.search(r'\b'+name+r'\b',relocations)
run(tool+'objcopy',*['--strip-symbol='+name for name in dead],raw,capsule)
assert subprocess.check_output([tool+'nm','-u',str(capsule)],text=True).split()==['U','scalar_fault_park']
layout=(here/'blackhole_qwen38_global_chain.ld').read_text();anchor=' .text 0x40000'
assert layout.count(anchor)==1
# New,explicit code DATA below the unchanged stack. Existing native code
# journals do NOT cover this by assumption;whole native ownership is owed.
layout=layout.replace(anchor,' .prepared 0x6c000 : { *(.text.q38_prepared_q4_* .text.*PioneerQ38PreparedQ4RootLayout* .text.*PioneerQ38PreparedQ4Layout*) . = ALIGN(4); }\n'+anchor)
if counter_unroll:layout=layout.replace('*(.text.q38_prepared_q4_*','*(.prepared.transfer .text.q38_prepared_q4_*')
layout=layout.replace(' ASSERT(SIZEOF(.forbidden)==0',
 ' ASSERT(SIZEOF(.prepared)>0 && SIZEOF(.prepared)<=0x2000,"distinct prepared ROOT code below stack;NO native grant")\n ASSERT(SIZEOF(.forbidden)==0')
(out/'program.ld').write_text(layout)
helpers=[]
if phase_profile:
 # Reproduce the ENTIRE unprofiled prepared-counter image FIRST. Do not
 # silently replace an accepted image or infer that old clock/native roles
 # authorize this new complete model revision.
 unchanged=out/'entire-unprofiled-prepared-root.elf'
 run(tool+'ld','-m','elf32lriscv','--no-relax','--gc-sections','-T',out/'program.ld',entry,objects[2],capsule,
  feedback,'-o',unchanged)
 assert hashlib.sha256(unchanged.read_bytes()).hexdigest()=='38a8f64f9bf75946953d87ec08d3bba07b44ea36303936ab912c89771f68d039'
 command=(out/'compile-token-argv.txt').read_text().splitlines()
 command[1:1]=['-DPIONEER_Q38_TOKEN_PHASE_PROFILE_IMAGE=1']
 token=out/'phase-token.o';command[command.index('-o')+1]=str(token);run(*command);objects[2]=token
 (out/'compile-profile-token-argv.txt').write_text('\n'.join(command)+'\n')
 helper=out/'phase-helper.o';command[command.index('-c')+1]=str(here/'blackhole_qwen38_token_phase_profile.cpp')
 command[command.index('-o')+1]=str(helper);command[command.index('-O2')]='-Os';run(*command);helpers.append(helper)
 (out/'compile-profile-helper-argv.txt').write_text('\n'.join(command)+'\n')
 for obj in (token,helper):
  frames=[row.split('\t') for row in obj.with_suffix('.su').read_text().splitlines()]
  assert frames and all(len(row)==3 and row[2]=='static' and int(row[1])<=4096 for row in frames)
elf=out/'program.elf';run(tool+'ld','-m','elf32lriscv','--no-relax','--gc-sections','-T',out/'program.ld',entry,objects[2],*helpers,capsule,
 feedback,'-Map',out/'program.map','-o',elf)
assert not subprocess.check_output([tool+'nm','-u',str(elf)]).strip()
symbols=subprocess.check_output([tool+'nm','-n',str(elf)],text=True)
assert not re.search(r'^[0-9a-f]+ [bBdDgGsS] ',symbols,re.M)
for name,address in (('_start',0x6000),('scalar_entry',0x40000),('scalar_fault_park',0x60000),('q38_global_chain_stopped',0x60004)):
 assert re.search('^'+format(address,'08x')+' T '+name+'$',symbols,re.M),name
for name in ('q38_prepared_q4_read_shared','q38_prepared_q4_stage_shared','q38_prepared_q4_bind_shared'):
 locations=re.findall(r'^([0-9a-f]+) [TW] '+name+'$',symbols,re.M)
 assert len(locations)==1 and 0x6c000<=int(locations[0],16)<0x6e000,name
dis=subprocess.check_output([tool+'objdump','-d',str(elf)],text=True);intervals=[];functions=[]
for p in map(str.split,subprocess.check_output([tool+'objdump','-t',str(elf)],text=True).splitlines()):
 if len(p)==6 and p[2]=='F' and int(p[4],16):
  first,size=int(p[0],16),int(p[4],16);assert not (first|size)&3
  assert 0x40000<=first<first+size<=0x60000 or 0x64000<=first<first+size<=0x6e000
  intervals.append((first,first+size))
  functions.append((first,first+size,p[5]))
instructions=[];csr=[];phase_calls={}
for m in re.finditer(r'^\s*([0-9a-f]+):\s+([0-9a-f]{8})\s.*$',dis,re.M):
 if any(a<=int(m[1],16)<b for a,b in intervals):
  word=int(m[2],16);assert word&3==3
  if word&127==0x73:
   owners={name for first,last,name in functions if first<=int(m[1],16)<last}
   assert phase_profile and owners=={'q38_token_phase_clock'}
   assert (word>>12)&7==2 and not ((word>>15)&31) and (word>>7)&31
   csr.append(word>>20)
  call=re.search(r'\bjalr\b.*# [0-9a-f]+ <(q38_token_phase_[a-z]+)>',m[0])
  if call:
   owners={name for first,last,name in functions if first<=int(m[1],16)<last}
   assert phase_profile and len(owners)==1
   key=(next(iter(owners)),call[1]);phase_calls[key]=phase_calls.get(key,0)+1
  assert not (word&127==0x67 and not ((word>>15)&31));instructions.append(m[0])
assert csr==([0xb80,0xb00,0xb80] if phase_profile else [])
assert phase_calls==({
 ('q38_global_chain_main','q38_token_phase_begin'):1,
 ('q38_global_chain_main','q38_token_phase_mark'):3,
 ('_ZN11GlobalChainL6FinishEPKjj','q38_token_phase_mark'):1,
 ('_ZN11GlobalChainL6FinishEPKjj','q38_token_phase_complete'):1,
 ('_ZN11GlobalChainL7RunHeadEv','q38_token_phase_mark'):9,
 ('q38_token_phase_begin','q38_token_phase_clock'):1,
 ('q38_token_phase_mark','q38_token_phase_clock'):1,
 ('q38_token_phase_complete','q38_token_phase_clock'):1,
} if phase_profile else {}) # Emitted call sites,NOT full execution/ownership proof.
assert not re.search(r'\b(?:divu?|remu?|mulh[su]*|fdiv\.[sd]|fsqrt\.[sd]|fmadd|fmsub|fnmadd|fnmsub|fcvt\.wu?\.s|ebreak)\b','\n'.join(instructions))
rows=subprocess.check_output([tool+'objdump','-h',str(elf)],text=True).splitlines()
for i,row in enumerate(rows):
 m=re.match(r'\s*\d+\s+(\S+)\s+([0-9a-fA-F]+)\s+',row)
 if m and 'ALLOC' in rows[i+1]:assert m[1] in ('.boot','.text','.terminal','.conv','.prepared'),row
(out/'program.symbols').write_text(symbols);(out/'program.disassembly').write_text(dis)
(out/'build-argv.txt').write_text('\n'.join('\t'.join(c) for c in commands)+'\n')
for section in ('boot','text','terminal','conv','prepared'):run(tool+'objcopy','-O','binary','-j','.'+section,elf,out/(section+'.bin'))
for section in ('boot','terminal'):assert (out/(section+'.bin')).read_bytes()==(control/(section+'.bin')).read_bytes()
for name in ('boot.bin','text.bin','terminal.bin','conv.bin','prepared.bin','program.elf'):
 data=(out/name).read_bytes();assert data and not len(data)%4;print((name,len(data),hashlib.sha256(data).hexdigest()),flush=True)
print('INSPECT_BUILD_ONLY prepared ROOT staged authentic-index ledger+per-worker descriptors;NEW6c000_code_view_NOT_NATIVE_AUTHORIZED;'
 +('phaseProfile10Buckets_readOnly3CSR_NO_clockFrequencyAssumption;' if phase_profile else '')
 +'NO execution/source/card/model/TPS grant',flush=True)
