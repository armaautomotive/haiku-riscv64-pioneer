#!/usr/bin/env python3
"""Compile/inspect returning device layer components. NO boot/install/card IO."""
import hashlib
import os
import pathlib
import re
import subprocess
import sys
if len(sys.argv)!=3 or any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ):raise SystemExit(2)
sdk,out=map(pathlib.Path,sys.argv[1:]);here=pathlib.Path(__file__).resolve().parent
assert sdk.is_absolute() and out.is_absolute() and not out.exists() and not out.is_symlink()
cross=str(sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku');assert pathlib.Path(cross+'-g++').is_file()
out.mkdir(mode=0o700)
def run(*args):subprocess.run([str(a) for a in args],check=True)
flags=['-std=c++11','-O2','-Wall','-Wextra','-Werror','-ffreestanding','-fno-exceptions','-fno-rtti','-fno-stack-protector',
 '-fno-pic','-fno-pie','-ffp-contract=off','-ffunction-sections','-fdata-sections','-fno-unwind-tables','-fno-asynchronous-unwind-tables',
 '-msmall-data-limit=0','-mno-relax','-march=rv32imf','-mabi=ilp32','-fstack-usage','-ffixed-s11']
for kind,parent in (('recurrent','PIONEER_Q38_MIXED_RECURRENT_BLOCK_ROOT_IMAGE'),('dense','PIONEER_Q38_MIXED_LIVE_BLOCK_ROOT_IMAGE')):
 run(cross+'-g++',*flags,'-DPIONEER_Q38_LAYER_RETURN_COMPONENT_IMAGE=1','-DPIONEER_Q38_LAYER_COMPONENT_'+kind.upper()+'=1',
  '-D'+parent+'=1','-DPIONEER_Q38_SIGMOID_PRECISION_IMAGE=1','-c',here/('blackhole_qwen38_return_'+kind+'_layer.cpp'),'-o',out/(kind+'.o'))
run(cross+'-g++',*flags,'-DPIONEER_Q38_MIXED_RECURRENT_BLOCK_ROOT_IMAGE=1','-DPIONEER_Q38_SIGMOID_PRECISION_IMAGE=1',
 '-DPIONEER_Q38_MIXED_ROOT_CAPSULE_IMAGE=1','-Dscalar_main=q38_mixed_recurrent_root_conv',
 '-Dq38_mixed_root_seal=q38_layer_return_recurrent_seal',
 '-include',here/'PioneerQ38MixedRecurrentBlockRootImageRole.h','-c',here/'blackhole_qwen38_conv_batch_silu_precision.cpp','-o',out/'root_conv.o')
run(cross+'-objcopy','--prefix-alloc-sections','.conv',out/'root_conv.o',out/'root_conv_sections.o')
raw=out/'layer-components-raw.o';capsule=out/'layer-components-NOBOOT.o'
run(cross+'-ld','-m','elf32lriscv','--no-relax','-r','--gc-sections','-u','q38_return_recurrent_layer','-u','q38_return_dense_layer',
 out/'recurrent.o',out/'dense.o',out/'root_conv_sections.o','-o',raw)
# A relocatable ld -r keeps dead extern declarations after section GC. Prove
# no remaining relocation can call any old wait/STOP, THEN strip only those
# unused names. objcopy itself refuses stripping a referenced symbol.
relocations=subprocess.check_output([cross+'-objdump','-r',str(raw)],text=True)
dead=('mixed_recurrent_block_wait','mixed_recurrent_block_stopped','mixed_live_block_wait','mixed_live_block_stopped')
for name in dead:assert not re.search(r'\b'+name+r'\b',relocations),name
run(cross+'-objcopy',*['--strip-symbol='+name for name in dead],raw,capsule)
symbols=subprocess.check_output([cross+'-nm','-C',str(capsule)],text=True)
undefined=subprocess.check_output([cross+'-nm','-u',str(capsule)],text=True)
assert undefined.split()==['U','scalar_fault_park'],undefined
for required in ('q38_return_recurrent_layer','q38_return_dense_layer','q38_layer_return_recurrent_seal','q38_layer_return_dense_seal','q38_mixed_recurrent_root_conv'):
 assert re.search(r' T '+required+r'$',symbols,re.M),required
for forbidden in ('mixed_recurrent_block_main','mixed_live_block_main','mixed_recurrent_block_wait','mixed_live_block_wait',
 'mixed_recurrent_block_stopped','mixed_live_block_stopped','q38_mixed_root_seal','_start','scalar_entry'):
 assert not re.search(r'\b'+forbidden+r'\b',symbols),forbidden
assert not re.search(r'^[0-9a-f]+ [bBdDgGsS] ',symbols,re.M)
dis=subprocess.check_output([cross+'-objdump','-d',str(capsule)],text=True)
assert not re.search(r'\b(?:divu?|remu?|mulh[su]*|fdiv\.[sd]|fsqrt\.[sd]|fmadd|fmsub|fnmadd|fnmsub|fcvt\.wu?\.s|ebreak)\b',dis)
for line in dis.splitlines():
 m=re.match(r'\s*[0-9a-f]+:\s+([0-9a-f]{8})\s',line)
 if m:assert int(m[1],16)&127!=0x73,'no controller CSR'
reserved=[s for s in dis.splitlines() if re.search(r'\bmv\s+s11,\w+\s*$',s)]
assert len(reserved)==4,'exact two private bind/restore pairs; no component owns the parent loop'
frames=[int(s.split('\t')[1]) for name in ('recurrent','dense','root_conv') for s in (out/(name+'.su')).read_text().splitlines()]
assert frames and max(frames)<=4096
sections=subprocess.check_output([cross+'-objdump','-h',str(capsule)],text=True).splitlines();text=conv=0
for i,row in enumerate(sections):
 m=re.match(r'\s*\d+\s+(\S+)\s+([0-9a-fA-F]+)\s+',row)
 if not m or 'ALLOC' not in sections[i+1]:continue
 name,size=m[1],int(m[2],16)
 assert name.startswith(('.text','.rodata','.srodata','.conv')),name
 if name.startswith('.conv'):conv+=size
 else:text+=size
assert text<=0x20000 and conv<=0x8000,(text,conv,'unchanged ROOT/Conv memory bounds')
PINS=(('recurrent.o',145120,'4b1550f36d8bf6e225684db0a97ba62ad885817c1497d78226443b0c7867533f'),
 ('dense.o',115164,'29204d287e38fe0ead7fd24c2de44aad7b46d2bcb38e23daa4b55d1cebb6c8e9'),
 ('layer-components-NOBOOT.o',228380,'78cf19c422c773a8c9c47c180df78ecd66609e4614dd75ee2d66344cab0ea25c'))
for name,size,digest in PINS:
 data=(out/name).read_bytes();actual=hashlib.sha256(data).hexdigest()
 assert len(data)==size and actual==digest,name
 print(actual+' '+name+' bytes'+str(len(data)))
print('PASS both ACTUAL returning layer bodies retained, separate private seal types/s11 bind+restore, same worker-chain/hidden-slot ADDRESSES;ROOTbytes'+str(text)+'/131072 Convbytes'+str(conv)+'/32768 maxFrame'+str(max(frames)))
print('RELOCATABLE COMPONENTS ONLY/NO boot or host wait/STOP/old main/native ABI;sole unresolved fault park. NOT a connected pipeline,initialized state,token commit,physical/Work or throughput qualification. DO NOT INSTALL')
