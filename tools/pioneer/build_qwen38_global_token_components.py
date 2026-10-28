#!/usr/bin/env python3
"""Returning cross-token components ONLY. No boot, persistent parent or native launch."""
import hashlib
import os
import pathlib
import re
import subprocess
import sys
assert len(sys.argv) in (3,4) and (len(sys.argv)==3 or sys.argv[3]=='--inspect-candidate')
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
sdk,out=map(pathlib.Path,sys.argv[1:3]);here=pathlib.Path(__file__).resolve().parent
assert sdk.is_absolute() and out.is_absolute() and not out.exists() and not out.is_symlink()
tool=str(sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku-');assert pathlib.Path(tool+'g++').is_file()
out.mkdir(mode=0o700)
def run(*args):subprocess.run([str(a) for a in args],check=True)
# FIRST preserve a whole current global image and all three old returning
# object pins. No old default image gets the new admission flag.
run(sys.executable,here/'build_qwen38_global_chain_root.py',sdk,out/'unchanged-feedback','--feedback')
flags=['-std=c++11','-O2','-Wall','-Wextra','-Werror','-ffreestanding','-fno-exceptions','-fno-rtti','-fno-stack-protector',
 '-fno-pic','-fno-pie','-ffp-contract=off','-ffunction-sections','-fdata-sections','-fno-unwind-tables','-fno-asynchronous-unwind-tables',
 '-msmall-data-limit=0','-mno-relax','-march=rv32imf','-mabi=ilp32','-fstack-usage','-ffixed-s11']
run(tool+'g++',*flags,'-DPIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE=1','-DPIONEER_Q38_SIGMOID_PRECISION_IMAGE=1',
 '-DPIONEER_Q38_LAYER_RETURN_COMPONENT_IMAGE=1','-DPIONEER_Q38_LAYER_COMPONENT_RECURRENT=1',
 '-DPIONEER_Q38_MIXED_RECURRENT_BLOCK_ROOT_IMAGE=1','-DPIONEER_Q38_GLOBAL_CHAIN_SEAL_UNROLL=1',
 '-DPIONEER_Q38_GLOBAL_CHAIN_CROSS_TOKEN_COMPONENT=1','-c',here/'blackhole_qwen38_return_recurrent_layer.cpp','-o',out/'recurrent.o')
raw=out/'token-components-raw.o';capsule=out/'token-components-NOBOOT.o';old=out/'unchanged-feedback'
run(tool+'ld','-m','elf32lriscv','--no-relax','-r','--gc-sections','-u','q38_return_recurrent_layer','-u','q38_return_dense_layer',
 '-u','q38_global_chain_learned_rms','-u','q38_global_chain_quantize14',out/'recurrent.o',old/'dense.o',old/'root_conv_sections.o','-o',raw)
dead=('mixed_recurrent_block_wait','mixed_recurrent_block_stopped','mixed_live_block_wait','mixed_live_block_stopped')
relocations=subprocess.check_output([tool+'objdump','-r',str(raw)],text=True)
for name in dead:assert not re.search(r'\b'+name+r'\b',relocations),name
run(tool+'objcopy',*['--strip-symbol='+name for name in dead],raw,capsule)
symbols=subprocess.check_output([tool+'nm','-C',str(capsule)],text=True)
assert subprocess.check_output([tool+'nm','-u',str(capsule)],text=True).split()==['U','scalar_fault_park']
for name in ('q38_return_recurrent_layer','q38_return_dense_layer','q38_global_chain_learned_rms','q38_global_chain_quantize14'):
 assert re.search(r' T '+name+r'$',symbols,re.M),name
assert 'DensePartitionRoot::AdmitGlobalOutput(' in symbols,'the REAL returning admission path must retain the new code'
for name in (*dead,'mixed_recurrent_block_main','mixed_live_block_main','q38_global_chain_main','q38_run_final_dense_and_stop','_start','scalar_entry'):
 assert not re.search(r'\b'+name+r'\b',symbols),name
assert not re.search(r'^[0-9a-f]+ [bBdDgGsS] ',symbols,re.M)
dis=subprocess.check_output([tool+'objdump','-d',str(capsule)],text=True)
assert not re.search(r'\b(?:divu?|remu?|mulh[su]*|fdiv\.[sd]|fsqrt\.[sd]|fmadd|fmsub|fnmadd|fnmsub|fcvt\.wu?\.s|ebreak)\b',dis)
for line in dis.splitlines():
 m=re.match(r'\s*[0-9a-f]+:\s+([0-9a-f]{8})\s',line)
 if m:assert int(m[1],16)&127!=0x73,'no controller CSR'
assert len([s for s in dis.splitlines() if re.search(r'\bmv\s+s11,\w+\s*$',s)])==4,'two existing bind/restore pairs only'
frames=[int(line.split('\t')[1]) for path in (out/'recurrent.su',old/'dense.su',old/'root_conv.su') for line in path.read_text().splitlines()]
assert frames and max(frames)<=4096,max(frames)
sections=subprocess.check_output([tool+'objdump','-h',str(capsule)],text=True).splitlines();text=conv=0
for i,row in enumerate(sections):
 m=re.match(r'\s*\d+\s+(\S+)\s+([0-9a-fA-F]+)\s+',row)
 if not m or 'ALLOC' not in sections[i+1]:continue
 name,size=m[1],int(m[2],16);assert name.startswith(('.text','.rodata','.srodata','.conv')),name
 if name.startswith('.conv'):conv+=size
 else:text+=size
assert text<=131072 and conv<=32768,(text,conv)
(out/'components.symbols').write_text(symbols);(out/'components.disassembly').write_text(dis)
PINS=(('recurrent.o',159412,'3c4537b03dd01fb17b9d66bf12990222f8495c8d1b253d915880d06bc7b750f6'),
 ('token-components-NOBOOT.o',245868,'8e600b590deb326bd1d2f6690e06152a0026148ea85c6c6dd8f1651cd231bd3a'))
observed=[]
for name in ('recurrent.o','token-components-NOBOOT.o'):
 data=(out/name).read_bytes();item=(name,len(data),hashlib.sha256(data).hexdigest());observed.append(item);print(repr(item),flush=True)
if len(sys.argv)==3:assert PINS and tuple(observed)==PINS,'unqualified components; inspect/freeze first'
print('BUILD ONLY actual returning recurrent path includes strict previous-global output admission;ROOTbytes'+str(text)+'/131072 Conv'+str(conv)+'/32768 maxFrame'+str(max(frames))+
 ';NO boot/persistent parent/model token-state commit/native/physical/Work/TPS or combined-stack proof. DO NOT INSTALL')
