#!/usr/bin/env python3
"""Isolated retained-Conv-closure ROOT BUILD; no native selector or card IO."""
import hashlib
import os
import pathlib
import re
import runpy
import subprocess
import sys

PINS=(
 ('boot.bin',4,'32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca'),
 ('text.bin',130936,'7c5e233b9cf59351dbaf7ec821917b23746ba3d3077e41922e7180a765343693'),
 ('terminal.bin',8,'468eb84a74c067cc328b3fb3e51b768a8440cd67d292add55eef6861c9b24de2'),
 ('conv.bin',32688,'9df7840dfc64968f63574069ef94ce63ab2e17fe8593eceb5068b0afbcfcc5f0'),
 ('program.elf',190700,'48b2bf31d2eefc36e50dcaaa9202d7d92579496a5ab72e9a62c9d4cfb66250ee'),
) # Independent main/clean BUILD identity ONLY;never physical/Work authority.
args=sys.argv[1:];inspect='--inspect-candidate' in args
if inspect:args.remove('--inspect-candidate')
if len(args)!=2 or any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ) or (not inspect and not PINS):
    raise SystemExit(2)
sdk,out=map(pathlib.Path,args);here=pathlib.Path(__file__).resolve().parent
assert sdk.is_absolute() and out.is_absolute() and not out.exists() and not out.is_symlink()
out.mkdir(mode=0o700);saved=sys.argv[:]
try:
    sys.argv=[str(here/'build_qwen38_token_fault_marker_image.py'),str(sdk),str(out/'unchanged-frontier'),
              '--job-boot-fold-eight','--token-frontier']
    old=runpy.run_path(sys.argv[0],run_name='__main__')
finally:sys.argv=saved
# Reproduce the WHOLE frozen9515589d parent FIRST. New policy affects only
# the recurrent TU;normal/fault ROOT,all dense+numeric capsules stay old.
parent=old['parent'];components=parent['components'];tool=parent['tool'];flags=parent['flags']
def run(*values):subprocess.run(list(map(str,values)),check=True)
defines=['PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE','PIONEER_Q38_SIGMOID_PRECISION_IMAGE',
 'PIONEER_Q38_JOB_BOOT_FOLD_IMAGE','PIONEER_Q38_GLOBAL_CHAIN_PACKET_SEAL_UNROLL',
 'PIONEER_Q38_GLOBAL_CHAIN_PACKET_SEAL_UNROLL_EIGHT','PIONEER_Q38_GLOBAL_CHAIN_SINGLE_CLOSED_AUDIT',
 'PIONEER_Q38_GLOBAL_CHAIN_WORKER_SEALED_ADMISSION','PIONEER_Q38_GLOBAL_CHAIN_PUSHED_COMPLETION',
 'PIONEER_Q38_GLOBAL_CHAIN_PRECISE_ACTIVATION','PIONEER_Q38_GLOBAL_CHAIN_LOCAL_SOURCE_GUARDS',
 'PIONEER_Q38_GLOBAL_CHAIN_QUIET_UNROLL','PIONEER_Q38_GLOBAL_CHAIN_BOUNDARY_SEALS',
 'PIONEER_Q38_GLOBAL_CHAIN_F32_HIDDEN','PIONEER_Q38_LAYER_RETURN_COMPONENT_IMAGE',
 'PIONEER_Q38_LAYER_COMPONENT_RECURRENT','PIONEER_Q38_MIXED_RECURRENT_BLOCK_ROOT_IMAGE',
 'PIONEER_Q38_GLOBAL_CHAIN_SEAL_UNROLL','PIONEER_Q38_GLOBAL_CHAIN_CROSS_TOKEN_COMPONENT']
compile_args=[tool+'g++',*flags,*['-D'+key+'=1' for key in defines]]
control=out/'unchanged-recurrent.o'
run(*compile_args,'-c',here/'blackhole_qwen38_return_recurrent_layer.cpp','-o',control)
assert control.read_bytes()==(components/'recurrent.o').read_bytes(),'complete old recurrent compilation must reproduce'
obj=out/'retained-conv-closure-recurrent.o'
command=[*compile_args,'-DPIONEER_Q38_RETAINED_CONV_CLOSURE_IMAGE=1','-c',
         str(here/'blackhole_qwen38_return_recurrent_layer.cpp'),'-o',str(obj)]
run(*command);(out/'compile-argv.txt').write_text('\n'.join(command)+'\n')
assert obj.read_bytes()!=control.read_bytes(),'new BUILD policy must actually change emitted code'
frames=[int(row.split('\t')[1]) for row in obj.with_suffix('.su').read_text().splitlines()]
assert frames and max(frames)<=4096,max(frames)
raw=out/'retained-components-raw.o';capsule=out/'retained-components-NOBOOT.o'
run(tool+'ld','-m','elf32lriscv','--no-relax','-r','--gc-sections',
 '-u','q38_return_recurrent_layer','-u','q38_return_dense_layer',
 '-u','q38_global_chain_learned_rms','-u','q38_global_chain_quantize14',
 obj,components/'dense.o',components/'unchanged-feedback/root_conv_sections.o','-o',raw)
dead=('mixed_recurrent_block_wait','mixed_recurrent_block_stopped','mixed_live_block_wait','mixed_live_block_stopped')
relocations=subprocess.check_output([tool+'objdump','-r',str(raw)],text=True)
for name in dead:assert not re.search(r'\b'+name+r'\b',relocations),name
run(tool+'objcopy',*['--strip-symbol='+name for name in dead],raw,capsule)
assert subprocess.check_output([tool+'nm','-u',str(capsule)],text=True).split()==['U','scalar_fault_park']
run(tool+'ld','-m','elf32lriscv','--no-relax','--gc-sections','-T',here/'blackhole_qwen38_global_chain.ld',
 out/'unchanged-frontier/start.o',parent['out']/'program.o',capsule,
 components/'unchanged-feedback/feedback.o','-Map',out/'program.map','-o',out/'program.elf')
symbols=subprocess.check_output([tool+'nm','-n',str(out/'program.elf')],text=True)
demangled=subprocess.check_output([tool+'nm','-nC',str(out/'program.elf')],text=True)
assert not subprocess.check_output([tool+'nm','-u',str(out/'program.elf')],text=True).strip()
assert not re.search(r'^[0-9a-f]+ [bBdDgGsS] ',symbols,re.M)
for name,address in (('_start',0x6000),('scalar_entry',0x40000),('scalar_fault_park',0x60000),('q38_global_chain_stopped',0x60004)):
    assert re.search('^'+format(address,'08x')+' T '+name+'$',symbols,re.M),name
for name in ('MixedRecurrentCohort::StageGdnAfterClosedConv(', 'MixedRecurrentCohort::StageConv(',
             'MixedRecurrentCohort::Finish(', 'MixedRecurrentCohort::CounterSeal(',
             'MixedRecurrentCohort::BootSeal(', 'GlobalChain::PublishFrontier(', 'q38_token_fault_report'):
    assert name in demangled,name
assert 'MixedRecurrentCohort::StageGdn(' not in demangled
disassembly=subprocess.check_output([tool+'objdump','-d',str(out/'program.elf')],text=True)
intervals=[]
for line in subprocess.check_output([tool+'objdump','-t',str(out/'program.elf')],text=True).splitlines():
    p=line.split()
    if len(p)==6 and p[2]=='F' and int(p[4],16):
        start,size=int(p[0],16),int(p[4],16);assert not (start|size)&3
        assert (0x40000<=start and start+size<=0x60000) or (0x64000<=start and start+size<=0x6c000)
        intervals.append((start,start+size))
instructions=[]
for line in disassembly.splitlines():
    match=re.match(r'\s*([0-9a-f]+):\s+([0-9a-f]{8})\s',line)
    if not match or not any(begin<=int(match[1],16) and int(match[1],16)+4<=end for begin,end in intervals):continue
    word=int(match[2],16)
    assert word&3==3 and word&127!=0x73,'only existing32bit RV32IMF,no CSR'
    assert not (word&127==0x67 and not ((word>>15)&31)),'no absolute JALR through x0'
    instructions.append(line)
assert not re.search(r'\b(?:divu?|remu?|mulh[su]*|fdiv\.[sd]|fsqrt\.[sd]|fmadd|fmsub|fnmadd|fnmsub|fcvt\.wu?\.s|ebreak)\b','\n'.join(instructions))
assert len([s for s in instructions if re.search(r'\bmv\s+s11,\w+\s*$',s)])==5
(out/'program.symbols').write_text(symbols);(out/'program.disassembly').write_text(disassembly)
observed=[]
for section in ('boot','text','terminal','conv'):
    run(tool+'objcopy','-O','binary','-j','.'+section,out/'program.elf',out/(section+'.bin'))
for name in ('boot.bin','text.bin','terminal.bin','conv.bin','program.elf'):
    data=(out/name).read_bytes();row=(name,len(data),hashlib.sha256(data).hexdigest());observed.append(row);print(row,flush=True)
for name in ('boot.bin','terminal.bin'):assert (out/name).read_bytes()==(out/'unchanged-frontier'/name).read_bytes()
assert len((out/'text.bin').read_bytes())<=131072 and len((out/'conv.bin').read_bytes())<=32768
if not inspect:assert tuple(observed)==PINS,'new BUILD not independently frozen'
print('BUILD_ONLY retained SAME119 actual Conv closure->GDN preparation/old951+complete recurrent object unchanged/'
      'current cold audit+ALL Conv/GDN completion source+counter+job+summary checks preserved;'
      'duplicate96 warm pre-GDN audits omitted ONLY in separately selected returning TU;'
      'NO native program DATA,driver,physical,original-model numerics,combined-stack,Work or TPS grant',flush=True)
