#!/usr/bin/env python3
"""NEW full returning BF16/FPU worker BUILD. No native install/device IO."""
import hashlib
import os
from pathlib import Path
import re
import runpy
import subprocess
import sys
assert __debug__ and len(sys.argv)==3 and not os.environ.get('PYTHONOPTIMIZE')
sdk,out=map(Path,sys.argv[1:]);here=Path(__file__).resolve().parent
assert sdk.is_absolute() and sdk.is_dir() and out.is_absolute() and not out.exists() and not out.is_symlink()
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
out.mkdir(mode=0o700);saved=sys.argv[:]
try:
 sys.argv=[str(here/'build_qwen38_global_worker_image.py'),str(sdk),str(out/'unchanged-global')]
 parent=runpy.run_path(sys.argv[0],run_name='__main__')
finally:sys.argv=saved
tool=str(parent['cross'])+'-';flags=parent['flags']
def run(*args):subprocess.run(list(map(str,args)),check=True)
for source,name,opt in (('blackhole_qwen38_global_worker.cpp','global','-Os'),('blackhole_q38_bf16_persistent_producer.cpp','bf16','-O2')):
 run(tool+'g++',*flags,opt,'-ffixed-s11','-DPIONEER_Q38_BF16_WORKER_IMAGE=1','-c',here/source,'-o',out/(name+'.o'))
run(tool+'g++','-std=c++17','-Os','-Wall','-Wextra','-Werror','-ffreestanding','-fno-builtin',
 '-fno-tree-loop-distribute-patterns','-fno-exceptions','-fno-rtti','-fno-stack-protector','-fno-pic','-fno-pie',
 '-ffunction-sections','-fdata-sections','-fno-unwind-tables','-fno-asynchronous-unwind-tables','-msmall-data-limit=0',
 '-mno-relax','-march=rv32i','-mabi=ilp32','-ffixed-s11','-fstack-usage','-Dcached_fpu_fault=block_fault_park',
 '-DPIONEER_Q38_CACHED_FPU_MATRIX_IMAGE=1','-DPIONEER_Q38_CACHED_FPU_RETURN_ONLY=1',
 '-c',here/'blackhole_q38_cached_fpu_matrix.cpp','-o',out/'cached.o')
assert set(row.split()[-1] for row in subprocess.check_output([tool+'nm','-u',str(out/'cached.o')],text=True).splitlines())=={'block_fault_park'},'cached leaf must not call byte libc memset'
run(tool+'as','-march=rv32imf','-mabi=ilp32','-mno-relax',here/'blackhole_q38_bf16_worker_start.S','-o',out/'start.o')
link=(out/'unchanged-global/program.ld').read_text();assert link.count('SECTIONS {')==1
link=link.replace('SECTIONS {','''SECTIONS {
 .bf16producer 0x30000 : { *bf16.o(.text .text.* .rodata .rodata.* .srodata .srodata.*) . = ALIGN(4); }
 .cachedfpu 0x50000 : { *cached.o(.text .text.* .rodata .rodata.* .srodata .srodata.*) . = ALIGN(4); }
 ASSERT(SIZEOF(.bf16producer)>0 && SIZEOF(.bf16producer)<=0x10000,"BF16 producer exceeds NEW code extent")
 ASSERT(SIZEOF(.cachedfpu)>0 && SIZEOF(.cachedfpu)<=0x10000,"Q4 FPU exceeds NEW code extent")''')
(out/'program.ld').write_text(link)
run(tool+'ld','-m','elf32lriscv','--no-relax','--gc-sections','-T',out/'program.ld',out/'start.o',out/'global.o',out/'bf16.o',out/'cached.o',
 *[out/'unchanged-global/qualified-math'/(name+'.o') for name in parent['units']],'-Map',out/'program.map','-o',out/'program.elf')
sections=('boot','globalworker','bf16producer','cachedfpu','dense','text','affine','ffn','service','gdn')
for section in sections:run(tool+'objcopy','-O','binary','-j','.'+section,out/'program.elf',out/(section+'.bin'))
symbols=subprocess.check_output([tool+'nm','-n',str(out/'program.elf')],text=True)
assert not subprocess.check_output([tool+'nm','-u',str(out/'program.elf')]).strip()
assert not re.search(r'^[0-9a-f]+ [bBdDgGsS] ',symbols,re.M)
for name in ('q38_global_worker_main','q38_mixed_seal','q38_bf16_persistent_producer','q38_cached_fpu_matrix'):
 assert re.search(r' T '+name+'$',symbols,re.M),name
dis=subprocess.check_output([tool+'objdump','-d',str(out/'program.elf')],text=True)
assert not re.search(r'\b(?:divu?|remu?|mulh[su]*|fdiv\.[sd]|fsqrt\.[sd]|fmadd|fmsub|fnmadd|fnmsub|fcvt\.wu?\.s|ebreak)\b',dis)
reserved=[row for row in dis.splitlines() if re.search(r'\bs11\b',row)]
assert len(reserved)==2 and sum(bool(re.search(r'\bmv\s+s11,\w+\s*$',r)) for r in reserved)==1
assert sum(bool(re.search(r'\bmv\s+\w+,s11\s*$',r)) for r in reserved)==1
for name in ('global','bf16','cached'):
 frames=[row.split('\t') for row in (out/(name+'.su')).read_text().splitlines()]
 assert frames and all(len(row)==3 and row[2]=='static' and int(row[1])<=4096 for row in frames)
(out/'program.symbols').write_text(symbols);(out/'program.disassembly').write_text(dis)
run(sys.executable,here/'tests/pioneer_q38_gdn_rendezvous_csr_gate.py',out/'program.disassembly')
rows=subprocess.check_output([tool+'objdump','-h',str(out/'program.elf')],text=True).splitlines()
for i,row in enumerate(rows):
 match=re.match(r'\s*\d+\s+(\S+)\s+([0-9a-fA-F]+)\s+',row)
 if match and 'ALLOC' in rows[i+1]:assert match[1][1:] in sections,row
role='PIONEER_Q38_BF16_WORKER_IMAGE'
negative=[('-U'+role,),('-U'+role,'-D'+role+'=0'),('-U'+role,'-D'+role+'=2'),('-ffast-math',)]
for macro in ('_KERNEL_MODE','PIONEER_DRIVER_BUILD_TAG','PIONEER_Q38_FULL_OFFLOAD_NATIVE','PIONEER_Q38_FULL_OFFLOAD_CLIENT',
 'PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_FULL_MODEL_CPU_REFERENCE','PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE',
 'PIONEER_Q38_GLOBAL_WORKER_IMAGE','PIONEER_Q38_PRECISE_WORKER_IMAGE','PIONEER_Q38_PUSH_COMPLETION_WORKER_IMAGE',
 'PIONEER_Q38_CACHED_FPU_MATRIX_IMAGE','PIONEER_NATIVE_COMPACT_Q4_STREAM_DIAGNOSTIC'):
 for value in (0,1):negative.append(('-D'+macro+'='+str(value),))
refusals=[]
for source in ('blackhole_qwen38_global_worker.cpp','blackhole_q38_bf16_persistent_producer.cpp'):
 for extra in negative:
  result=subprocess.run([tool+'g++',*flags,'-D'+role+'=1',*extra,'-E',str(here/source)],stdout=subprocess.DEVNULL,stderr=subprocess.PIPE)
  assert result.returncode and b'error:' in result.stderr,(source,extra)
  refusals.append('PASS '+source+' '+' '.join(extra))
(out/'role-refusals.txt').write_text('\n'.join(refusals)+'\n')
for section in sections:
 data=(out/(section+'.bin')).read_bytes();assert data and not len(data)%4
 print('BF16_WORKER_BUILD',section,len(data),hashlib.sha256(data).hexdigest(),flush=True)
print('PASS BUILD_ONLY QGW4 returning producer/typed BF16/paired Q4 FPU/old non-Q4 capsules/private seal/roles'+str(len(refusals))+';NO ROOT/native/owner/model-quality/hardware/TPS authority')
