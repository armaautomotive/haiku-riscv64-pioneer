#!/usr/bin/env python3
"""Isolated ALL-ROOT-TU flat QGW3 grammar BUILD;no native/device/Work grant."""
import hashlib
import os
import pathlib
import re
import runpy
import subprocess
import sys

PINS = (
    ('boot.bin',4,'32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca'),
    ('text.bin',129684,'c1a8a1c020d895f5e9e3a2530e2ff686c16f4c01f0c01057cccc91d818b85504'),
    ('terminal.bin',8,'468eb84a74c067cc328b3fb3e51b768a8440cd67d292add55eef6861c9b24de2'),
    ('conv.bin',32692,'ef9a1b0de3baf346b893f3dc0db2363e11d19a465f2d39ecc313cc06730e3066'),
    ('program.elf',191436,'88e419152d297dbe2ebb0b3913d3e2cae58aa69741b4b03e2667749702c4a674'),
)  # Main/clean BUILD identity ONLY,not execution/native/Work permission.
args=sys.argv[1:]
inspect='--inspect-candidate' in args
if inspect:
    assert args.count('--inspect-candidate')==1
    args.remove('--inspect-candidate')
assert len(args)==2 and (inspect or PINS)
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
sdk,out=map(pathlib.Path,args)
here=pathlib.Path(__file__).resolve().parent
assert sdk.is_absolute() and out.is_absolute() and not out.exists() and not out.is_symlink()
out.mkdir(mode=0o700)
saved=sys.argv[:]
try:
    sys.argv=[str(here/'build_qwen38_empty_receipt_poll_root.py'),str(sdk),str(out/'unchanged-empty')]
    base=runpy.run_path(sys.argv[0],run_name='__main__')
finally:
    sys.argv=saved


def run(*values):
    subprocess.run(list(map(str,values)),check=True)


tool=base['tool']
flag='-DPIONEER_Q38_FLAT_MATRIX_GRAMMAR_IMAGE=1'
objects=[]
commands=[]
# The whole3718 control,including all ancestors,already reproduced FIRST.
# Compile EVERY TU that can emit the shared QGW3 grammar with the SAME flag;
# never let ld choose an old/new COMDAT body depending on object order.
old_command=list(map(str,base['command']))
control=out/'unchanged-recurrent.o'
run(*old_command[:-1],control)
assert control.read_bytes()==base['obj'].read_bytes()
recurrent=out/'flat-recurrent.o'
command=[old_command[0],flag,*old_command[1:-1],str(recurrent)]
run(*command);objects.append(recurrent);commands.append(command)

common=[v for v in old_command[:old_command.index('-c')] if not v.startswith('-D')]
dense_defines=('GLOBAL_CHAIN_COMPONENT_IMAGE','SIGMOID_PRECISION_IMAGE','JOB_BOOT_FOLD_IMAGE',
 'GLOBAL_CHAIN_SINGLE_CLOSED_AUDIT','GLOBAL_CHAIN_WORKER_SEALED_ADMISSION','GLOBAL_CHAIN_PUSHED_COMPLETION',
 'GLOBAL_CHAIN_PRECISE_ACTIVATION','GLOBAL_CHAIN_LOCAL_SOURCE_GUARDS','GLOBAL_CHAIN_QUIET_UNROLL',
 'GLOBAL_CHAIN_BOUNDARY_SEALS','GLOBAL_CHAIN_F32_HIDDEN','GLOBAL_CHAIN_SEAL_UNROLL',
 'LAYER_RETURN_COMPONENT_IMAGE','LAYER_COMPONENT_DENSE','MIXED_LIVE_BLOCK_ROOT_IMAGE','GLOBAL_CHAIN_TOKEN_KV_APPEND')
dense_command=[*common,*['-DPIONEER_Q38_'+v+'=1' for v in dense_defines],
               '-c',str(here/'blackhole_qwen38_return_dense_layer.cpp'),'-o']
control=out/'unchanged-dense.o'
run(*dense_command,control)
components=base['components']
assert control.read_bytes()==(components/'dense.o').read_bytes(),'ENTIRE old dense object must reproduce'
dense=out/'flat-dense.o'
command=[dense_command[0],flag,*dense_command[1:],str(dense)]
run(*command);objects.append(dense);commands.append(command)

old_commit=base['commit_object']
commit_command=(old_commit.parent/'compile-argv.txt').read_text().splitlines()
assert commit_command[0]==tool+'g++' and commit_command.count('-c')==commit_command.count('-o')==1
assert pathlib.Path(commit_command[commit_command.index('-c')+1]).name=='blackhole_qwen38_token_root.cpp'
assert pathlib.Path(commit_command[commit_command.index('-o')+1])==old_commit
commit_command[commit_command.index('-c')+1]=str(here/'blackhole_qwen38_token_root.cpp')
control=out/'unchanged-token.o'
commit_command[commit_command.index('-o')+1]=str(control)
run(*commit_command)
assert control.read_bytes()==old_commit.read_bytes(),'ENTIRE old token object must reproduce'
token=out/'flat-token.o'
commit_command[commit_command.index('-o')+1]=str(token)
command=[commit_command[0],flag,*commit_command[1:]]
run(*command);objects.append(token);commands.append(command)
for kind,command in zip(('recurrent','dense','token'),commands):
    (out/('compile-'+kind+'-argv.txt')).write_text('\n'.join(command)+'\n')
for obj in objects:
    frames=[int(row.split('\t')[1]) for row in obj.with_suffix('.su').read_text().splitlines()]
    assert frames and max(frames)<=4096,(obj,max(frames))
for old in (components/'unchanged-feedback/root_conv_sections.o',components/'unchanged-feedback/feedback.o'):
    table=subprocess.check_output([tool+'nm','-C',str(old)],text=True)
    assert 'PioneerQ38PushedWorkerLayout::' not in table,'unconverted TU could emit an old grammar body'

raw,capsule=out/'components-raw.o',out/'components-NOBOOT.o'
run(tool+'ld','-m','elf32lriscv','--no-relax','-r','--gc-sections',
 '-u','q38_return_recurrent_layer','-u','q38_return_dense_layer',
 '-u','q38_global_chain_learned_rms','-u','q38_global_chain_quantize14',
 recurrent,dense,components/'unchanged-feedback/root_conv_sections.o','-o',raw)
dead=('mixed_recurrent_block_wait','mixed_recurrent_block_stopped','mixed_live_block_wait','mixed_live_block_stopped')
relocations=subprocess.check_output([tool+'objdump','-r',str(raw)],text=True)
for name in dead:assert not re.search(r'\b'+name+r'\b',relocations),name
run(tool+'objcopy',*['--strip-symbol='+name for name in dead],raw,capsule)
assert subprocess.check_output([tool+'nm','-u',str(capsule)],text=True).split()==['U','scalar_fault_park']
elf=out/'program.elf'
run(tool+'ld','-m','elf32lriscv','--no-relax','--gc-sections','-T',here/'blackhole_qwen38_global_chain.ld',
 base['entry_object'],token,capsule,components/'unchanged-feedback/feedback.o','-Map',out/'program.map','-o',elf)
assert not subprocess.check_output([tool+'nm','-u',str(elf)],text=True).strip()
symbols=subprocess.check_output([tool+'nm','-n',str(elf)],text=True)
demangled=subprocess.check_output([tool+'nm','-nC',str(elf)],text=True)
assert not re.search(r'^[0-9a-f]+ [bBdDgGsS] ',symbols,re.M)
for name,address in (('_start',0x6000),('scalar_entry',0x40000),('scalar_fault_park',0x60000),('q38_global_chain_stopped',0x60004)):
    assert re.search('^'+format(address,'08x')+' T '+name+'$',symbols,re.M),name
for name in ('PioneerQ38PushedWorkerLayout::JobShape(','PioneerQ38PushedWorkerLayout::BootShape(',
             'PioneerQ38CounterGather::Read(','q38_layer_return_recurrent_empty_receipt_seal','GlobalChain::CommitState('):
    assert name in demangled,name
assert 'PioneerQ38PreciseWorkerLayout::JobShape(' not in demangled,'old copied-array fallback retained'
assert 'PioneerQ38PreciseWorkerLayout::BootShape(' not in demangled,'old copied-array boot fallback retained'
disassembly=subprocess.check_output([tool+'objdump','-d',str(elf)],text=True)
intervals=[]
for line in subprocess.check_output([tool+'objdump','-t',str(elf)],text=True).splitlines():
    row=line.split()
    if len(row)==6 and row[2]=='F' and int(row[4],16):
        first,size=int(row[0],16),int(row[4],16)
        assert not (first|size)&3
        assert 0x40000<=first<first+size<=0x60000 or 0x64000<=first<first+size<=0x6c000
        intervals.append((first,first+size))
instructions=[]
for line in disassembly.splitlines():
    match=re.match(r'\s*([0-9a-f]+):\s+([0-9a-f]{8})\s',line)
    if match and any(a<=int(match[1],16)<b for a,b in intervals):
        word=int(match[2],16)
        assert word&3==3 and word&127!=0x73,'RV32IMF only,no CSR'
        assert not (word&127==0x67 and not ((word>>15)&31)),'no absolute JALR'
        instructions.append(line)
assert not re.search(r'\b(?:divu?|remu?|mulh[su]*|fdiv\.[sd]|fsqrt\.[sd]|fmadd|fmsub|fnmadd|fnmsub|fcvt\.wu?\.s|ebreak)\b','\n'.join(instructions))
(out/'program.symbols').write_text(symbols)
(out/'program.disassembly').write_text(disassembly)
observed=[]
for section in ('boot','text','terminal','conv'):
    run(tool+'objcopy','-O','binary','-j','.'+section,elf,out/(section+'.bin'))
for name in ('boot.bin','text.bin','terminal.bin','conv.bin','program.elf'):
    data=(out/name).read_bytes();row=(name,len(data),hashlib.sha256(data).hexdigest());observed.append(row);print(row,flush=True)
for name in ('boot.bin','terminal.bin'):assert (out/name).read_bytes()==(base['out']/name).read_bytes()
assert len((out/'text.bin').read_bytes())<=131072 and len((out/'conv.bin').read_bytes())<=32768
if not inspect:assert tuple(observed)==PINS,'candidate not independently frozen'
print('BUILD_ONLY ALL3 grammar-emitting ROOT TUs selected consistently/full old3718+ancestors+3objects FIRST/'
      'same QGW3 DATA predicate without normalization arrays/no old Precise boot/job fallback;'
      'NOT emitted-argument,connected-model,native,hardware,Work or TPS qualification',flush=True)
