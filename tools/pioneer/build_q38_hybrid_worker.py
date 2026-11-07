#!/usr/bin/env python3
"""Build experimental QGW5 worker only; never installs or accesses a device."""
import hashlib
import os
from pathlib import Path
import re
import runpy
import subprocess
import sys

options=sys.argv[3:]
assert __debug__ and len(sys.argv)>=3 and len(options)==len(set(options)) and set(options)<={'--test-bfp8-layer0','--reuse-input'}
test_layer0='--test-bfp8-layer0' in options;reuse='--reuse-input' in options
assert not os.environ.get('PYTHONOPTIMIZE')
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
sdk,out=map(Path,sys.argv[1:3]);here=Path(__file__).resolve().parent
assert sdk.is_absolute() and sdk.is_dir() and out.is_absolute() and not out.exists() and not out.is_symlink()
out.mkdir(mode=0o700)
saved=sys.argv[:]
try:
 sys.argv=[str(here/'build_q38_bf16_worker.py'),str(sdk),str(out/'baseline')]
 baseline=runpy.run_path(sys.argv[0],run_name='__main__')
finally:sys.argv=saved
tool=str(baseline['tool']);flags=baseline['flags']
def run(*args):subprocess.run([str(a) for a in args],check=True)
role=['-DPIONEER_Q38_BF16_WORKER_IMAGE=1','-DPIONEER_Q38_HYBRID_WORKER_IMAGE=1','-DPIONEER_DRAM_SWEEP_FREESTANDING=1']
if test_layer0:role.append('-DPIONEER_Q38_HYBRID_TEST_BFP8_LAYER0=1')
if reuse:role.append('-DPIONEER_Q38_HYBRID_INPUT_REUSE_INSPECTION=1')
run(tool+'g++',*flags,'-Os','-ffixed-s11',*role,'-c',here/'blackhole_qwen38_global_worker.cpp','-o',out/'global.o')
for bits in (4,8):
 run(tool+'g++','-std=c++11','-O2','-Wall','-Wextra','-Werror','-ffreestanding','-fno-builtin',
  '-fno-exceptions','-fno-rtti','-fno-stack-protector','-fno-pic','-fno-pie',
  '-ffunction-sections','-fdata-sections','-fno-unwind-tables','-fno-asynchronous-unwind-tables',
  '-msmall-data-limit=0','-mno-relax','-march=rv32i_zicsr','-mabi=ilp32','-ffixed-s11','-fstack-usage',
  '-DPIONEER_Q38_HYBRID_WORKER_IMAGE=1','-Dnative_dram_band_fault=block_fault_park',
  *(['-DPIONEER_Q38_HYBRID_INPUT_REUSE_INSPECTION=1','-DPIONEER_Q38_BFP4_INPUT_REUSE_EXPERIMENT=1'] if reuse and bits==4 else []),
  '-Dnative_dram_band_main=q38_bfp'+str(bits)+'_worker_leaf',
  '-c',here/('blackhole_q38_bfp'+str(bits)+'_worker_leaf.cpp'),'-o',out/('bfp'+str(bits)+'.o'))
link=(out/'baseline/program.ld').read_text()
# The original GDN image at 0xb8000 overlaps BFP weight staging, including
# BFP8 K5120 and BFP4 K17408. This experimental placement is NOT a native
# ownership or save/restore grant; complete peer-L1 qualification remains.
assert link.count('.gdn 0xb8000')==1
link=link.replace('.gdn 0xb8000','.gdn 0x170000')
link=link.replace('SIZEOF(.gdn)<=0x10000','SIZEOF(.gdn)<=0xffc0')
assert 0x90000-64+544*1152 <= 0x12c000-64
assert 0x12c000-64+544*128 <= 0x140000
assert 0x140000+256 <= 0x170000 and 0x170000+0xffc0 <= 0x17ffc0
old='*cached.o(.text .text.* .rodata .rodata.* .srodata .srodata.*)'
assert link.count(old)==1
link=link.replace(old,'*bfp4.o(.text .text.* .rodata .rodata.* .srodata .srodata.*) *bfp8.o(.text .text.* .rodata .rodata.* .srodata .srodata.*)')
(out/'program.ld').write_text(link)
run(tool+'ld','-m','elf32lriscv','--no-relax','--gc-sections','-T',out/'program.ld',out/'baseline/start.o',out/'global.o',
 out/'baseline/bf16.o',out/'bfp4.o',out/'bfp8.o',
 *[out/'baseline/unchanged-global/qualified-math'/(name+'.o') for name in baseline['parent']['units']],
 '-Map',out/'program.map','-o',out/'program.elf')
symbols=subprocess.check_output([tool+'nm','-n',str(out/'program.elf')],text=True)
assert not subprocess.check_output([tool+'nm','-u',str(out/'program.elf')]).strip()
assert not re.search(r'^[0-9a-f]+ [bBdDgGsS] ',symbols,re.M)
for name in ('q38_global_worker_main','q38_mixed_seal','q38_bfp4_worker_leaf','q38_bfp8_worker_leaf'):
 assert re.search(r' T '+name+'$',symbols,re.M),name
dis=subprocess.check_output([tool+'objdump','-d',str(out/'program.elf')],text=True)
assert not re.search(r'\b(?:divu?|remu?|mulh[su]*|fdiv\.[sd]|fsqrt\.[sd]|fmadd|fmsub|fnmadd|fnmsub|fcvt\.wu?\.s|ebreak)\b',dis)
reserved=[row for row in dis.splitlines() if re.search(r'\bs11\b',row)]
assert len(reserved)==2
for name in ('global','bfp4','bfp8'):
 frames=[row.split('\t') for row in (out/(name+'.su')).read_text().splitlines()]
 assert frames and all(len(row)==3 and row[2]=='static' and int(row[1])<=4096 for row in frames)
(out/'program.symbols').write_text(symbols);(out/'program.disassembly').write_text(dis)
for section in ('boot','globalworker','bf16producer','cachedfpu','dense','text','affine','ffn','service','gdn'):
 path=out/(section+'.bin');run(tool+'objcopy','-O','binary','-j','.'+section,out/'program.elf',path)
 data=path.read_bytes();assert data and not len(data)%4
 print('HYBRID_WORKER_BUILD',section,len(data),hashlib.sha256(data).hexdigest(),flush=True)
print('BUILD ONLY: experimental linked QGW5 dispatcher and BFP4/BFP8 leaves; NO ROOT, owner, model, hardware, quality or speed proof')
if test_layer0:print('TEST VARIANT ONLY: BFP8 layer0 mask; never install or use for inference')
if reuse:print('INPUT-REUSE INSPECTION ONLY: not qualified for native, Work, ownership or hardware')
