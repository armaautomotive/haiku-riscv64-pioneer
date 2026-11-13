#!/usr/bin/env python3
"""Build standalone RV32 alpha/beta plan experiment; no install or device IO."""
import hashlib
import os
from pathlib import Path
import re
import subprocess
import sys

assert __debug__ and len(sys.argv)==3 and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
sdk,out=map(Path,sys.argv[1:]);here=Path(__file__).resolve().parent
assert sdk.is_absolute() and sdk.is_dir() and not sdk.is_symlink()
assert out.is_absolute() and not out.exists() and not out.is_symlink()
out.mkdir(mode=0o700)
tool=str(sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku-')
roles=('PIONEER_DRAM_SWEEP_FREESTANDING','PIONEER_ATTENTION_FREESTANDING','PIONEER_Q38_RECURRENT_BFP8_WIDTH_INSPECTION',
 'PIONEER_Q38_RECURRENT_SMALL_BFP8_WORKER_INSPECTION','PIONEER_Q38_QGWF_BATCH4_WORKER','PIONEER_Q38_QGWE_BATCH_DENSE_WORKER',
 'PIONEER_Q38_QGW8_BFP8_BATCH_WORKER','PIONEER_Q38_BFP4_BATCH_INPUT_EXPERIMENT','PIONEER_Q38_BFP8_BATCH_INPUT_EXPERIMENT',
 'PIONEER_Q38_QGWD_SRCA_DENSE_Q8_WORKER','PIONEER_Q38_DENSE_Q8_MATRIX_WORKER_INSPECTION',
 'PIONEER_Q38_DENSE_BF16_SRCA_BASE_CANDIDATE','PIONEER_Q38_HEAD_MATRIX_WORKER_INSPECTION','PIONEER_Q38_HYBRID_WORKER_IMAGE',
 'PIONEER_Q38_BF16_WORKER_IMAGE','PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION','PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT')
flags=['-std=c++11','-Os','-Wall','-Wextra','-Werror','-ffreestanding','-fno-exceptions','-fno-rtti','-fno-stack-protector',
 '-fno-pic','-fno-pie','-fno-builtin','-ffp-contract=off','-ffunction-sections','-fdata-sections','-fno-unwind-tables',
 '-fno-asynchronous-unwind-tables','-msmall-data-limit=0','-mno-relax','-march=rv32imf','-mabi=ilp32','-fstack-usage']
subprocess.run([tool+'g++',*flags,*['-D'+r+'=1' for r in roles],'-c',str(here/'blackhole_q38_recurrent_small_plan.cpp'),'-o',str(out/'program.o')],check=True)
subprocess.run([tool+'ld','-m','elf32lriscv','--no-relax','-Ttext=0x40000','-e','q38_small_plan',str(out/'program.o'),'-o',str(out/'program.elf')],check=True)
assert not subprocess.check_output([tool+'nm','-u',str(out/'program.elf')]).strip()
symbols=subprocess.check_output([tool+'nm','-n',str(out/'program.elf')],text=True)
for name in ('q38_small_plan','q38_small_copy','memcpy','memset'):assert re.search(r' T '+name+'$',symbols,re.M)
assert not re.search(r'^[0-9a-f]+ [bBdDgGsS] ',symbols,re.M)
(out/'program.symbols').write_text(symbols)
dis=subprocess.check_output([tool+'objdump','-d',str(out/'program.elf')],text=True)
assert not re.search(r'\b(?:divu?|remu?|mulh[su]*|fdiv\.[sd]|fsqrt\.[sd]|fmadd|fmsub|fnmadd|fnmsub|fcvt\.wu?\.s|ebreak|sb|sh)\b',dis)
for line in dis.splitlines():
    instruction=re.match(r'\s*[0-9a-f]+:\s+([0-9a-f]{8})\s',line)
    if instruction:assert int(instruction[1],16)&127!=0x73
sections=subprocess.check_output([tool+'objdump','-h',str(out/'program.elf')],text=True).splitlines()
for i,line in enumerate(sections):
    match=re.match(r'\s*\d+\s+(\S+)\s+([0-9a-fA-F]+)\s+',line)
    if match and 'ALLOC' in sections[i+1]:assert match[1] in ('.text','.rodata'),match[1]
subprocess.run([tool+'objcopy','-O','binary','-j','.text','-j','.rodata',str(out/'program.elf'),str(out/'text.bin')],check=True)
data=(out/'text.bin').read_bytes();assert data and not len(data)%4 and len(data)<=32768
frames=[row.split('\t') for row in (out/'program.su').read_text().splitlines()]
assert frames and all(len(row)==3 and row[2]=='static' and int(row[1])<=2048 for row in frames)
print('BUILT standalone RV32 small projection plan text',len(data),'sha256',hashlib.sha256(data).hexdigest(),'; NO ROOT lifecycle, native, device or speed')
