#!/usr/bin/env python3
"""Matching batched-prefetch ROOT BUILD only;NO old DATA/native authority."""
import hashlib
import os
import pathlib
import re
import runpy
import subprocess
import sys

PINS=(
    ('boot.bin',4,'32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca'),
    ('text.bin',129672,'fc2674af71846a63bca556f3ef3a4128154d246ea7a1fdaa2665fe00c713e0dd'),
    ('terminal.bin',8,'468eb84a74c067cc328b3fb3e51b768a8440cd67d292add55eef6861c9b24de2'),
    ('conv.bin',32692,'19f59e9c75feb02f247e0c3ce1feb8cd3691d79ba5654484cc5ebed2451b0315'),
    ('program.elf',191436,'6066e6935f7230175aab0ac98379cdc2d58dc35e49a15ca89af5064797e2d48b'),
)  # Independent whole-artifact BUILD identity ONLY,not execution authority.
args=sys.argv[1:];inspect='--inspect-candidate' in args
if inspect:
    assert args.count('--inspect-candidate')==1;args.remove('--inspect-candidate')
assert len(args)==2 and (inspect or PINS)
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
sdk,out=map(pathlib.Path,args);here=pathlib.Path(__file__).resolve().parent
assert sdk.is_absolute() and out.is_absolute() and not out.exists() and not out.is_symlink()
out.mkdir(mode=0o700);saved=sys.argv[:]
try:
    sys.argv=[str(here/'build_qwen38_flat_matrix_grammar_root.py'),str(sdk),str(out/'unchanged-flat')]
    parent=runpy.run_path(sys.argv[0],run_name='__main__')
finally:
    sys.argv=saved
assert hashlib.sha256((out/'unchanged-flat/program.elf').read_bytes()).hexdigest()=='88e419152d297dbe2ebb0b3913d3e2cae58aa69741b4b03e2667749702c4a674'
tool=parent['tool']
def run(*values):subprocess.run(list(map(str,values)),check=True)
objects=[]
for kind,old in zip(('recurrent','dense','token'),parent['objects']):
    command=(out/'unchanged-flat'/('compile-'+kind+'-argv.txt')).read_text().splitlines()
    assert command[0]==tool+'g++' and command.count('-c')==command.count('-o')==1
    source=here/pathlib.Path(command[command.index('-c')+1]).name
    command[command.index('-c')+1]=str(source)
    control=out/('unchanged-'+kind+'.o');command[command.index('-o')+1]=str(control)
    run(*command);assert control.read_bytes()==old.read_bytes(),'ENTIRE old ROOT TU must reproduce FIRST'
    obj=out/('batched-'+kind+'.o');command[command.index('-o')+1]=str(obj)
    command=[command[0],'-DPIONEER_Q38_BATCHED_PREFETCH_ROOT_IMAGE=1','-include',
             str(here/'PioneerQ38BatchedPrefetchRootRole.h'),*command[1:]]
    run(*command);objects.append(obj)
    (out/('compile-'+kind+'-argv.txt')).write_text('\n'.join(command)+'\n')
    frames=[int(row.split('\t')[1]) for row in obj.with_suffix('.su').read_text().splitlines()]
    assert frames and max(frames)<=4096,(obj,max(frames))
base=parent['base'];components=parent['components']
raw,capsule=out/'components-raw.o',out/'components-NOBOOT.o'
run(tool+'ld','-m','elf32lriscv','--no-relax','-r','--gc-sections',
    '-u','q38_return_recurrent_layer','-u','q38_return_dense_layer',
    '-u','q38_global_chain_learned_rms','-u','q38_global_chain_quantize14',
    objects[0],objects[1],components/'unchanged-feedback/root_conv_sections.o','-o',raw)
dead=('mixed_recurrent_block_wait','mixed_recurrent_block_stopped','mixed_live_block_wait','mixed_live_block_stopped')
relocations=subprocess.check_output([tool+'objdump','-r',str(raw)],text=True)
for name in dead:assert not re.search(r'\b'+name+r'\b',relocations)
run(tool+'objcopy',*['--strip-symbol='+name for name in dead],raw,capsule)
assert subprocess.check_output([tool+'nm','-u',str(capsule)],text=True).split()==['U','scalar_fault_park']
elf=out/'program.elf'
run(tool+'ld','-m','elf32lriscv','--no-relax','--gc-sections','-T',here/'blackhole_qwen38_global_chain.ld',
    base['entry_object'],objects[2],capsule,components/'unchanged-feedback/feedback.o','-Map',out/'program.map','-o',elf)
assert not subprocess.check_output([tool+'nm','-u',str(elf)],text=True).strip()
symbols=subprocess.check_output([tool+'nm','-n',str(elf)],text=True)
assert not re.search(r'^[0-9a-f]+ [bBdDgGsS] ',symbols,re.M)
for name,address in (('_start',0x6000),('scalar_entry',0x40000),('scalar_fault_park',0x60000),('q38_global_chain_stopped',0x60004)):
    assert re.search('^'+format(address,'08x')+' T '+name+'$',symbols,re.M)
disassembly=subprocess.check_output([tool+'objdump','-d',str(elf)],text=True)
intervals=[]
for row in map(str.split,subprocess.check_output([tool+'objdump','-t',str(elf)],text=True).splitlines()):
    if len(row)==6 and row[2]=='F' and int(row[4],16):
        first,size=int(row[0],16),int(row[4],16)
        assert not (first|size)&3 and (0x40000<=first<first+size<=0x60000 or 0x64000<=first<first+size<=0x6c000)
        intervals.append((first,first+size))
instructions=[]
for match in re.finditer(r'^\s*([0-9a-f]+):\s+([0-9a-f]{8})\s.*$',disassembly,re.M):
    if any(a<=int(match[1],16)<b for a,b in intervals):
        word=int(match[2],16)
        assert word&3==3 and word&127!=0x73
        assert not (word&127==0x67 and not ((word>>15)&31))
        instructions.append(match[0])
# Match the unchanged parent:objdump can disassemble embedded DATA at
# unaligned addresses as fictitious FMA opcodes. Only actual symbol-bounded
# functions are instruction authority;never enlarge intervals or drop an
# actual forbidden instruction to make this gate pass.
assert not re.search(r'\b(?:divu?|remu?|mulh[su]*|fdiv\.[sd]|fsqrt\.[sd]|fmadd|fmsub|fnmadd|fnmsub|fcvt\.wu?\.s|ebreak)\b','\n'.join(instructions))
rows=subprocess.check_output([tool+'objdump','-h',str(elf)],text=True).splitlines()
for index,row in enumerate(rows):
    match=re.match(r'\s*\d+\s+(\S+)\s+([0-9a-fA-F]+)\s+',row)
    if match and 'ALLOC' in rows[index+1]:assert match[1] in ('.boot','.text','.terminal','.conv')
(out/'program.symbols').write_text(symbols);(out/'program.disassembly').write_text(disassembly)
observed=[]
for section in ('boot','text','terminal','conv'):run(tool+'objcopy','-O','binary','-j','.'+section,elf,out/(section+'.bin'))
for name in ('boot.bin','text.bin','terminal.bin','conv.bin','program.elf'):
    data=(out/name).read_bytes();row=(name,len(data),hashlib.sha256(data).hexdigest());observed.append(row);print(row,flush=True)
for name in ('boot.bin','terminal.bin'):assert (out/name).read_bytes()==(out/'unchanged-flat'/name).read_bytes()
assert len((out/'text.bin').read_bytes())<=131072 and len((out/'conv.bin').read_bytes())<=32768
if not inspect:assert tuple(observed)==PINS
print('BUILD_ONLY matching batched counts in projections AND cohort Q8 re-admission AND all-vocabulary global head/'
      'ALL3 ROOT TUs same selection/full88e4+ancestors+3wholeTUs FIRST;NO emitted execution,old DATA/native,hardware,Work or TPS grant',flush=True)
