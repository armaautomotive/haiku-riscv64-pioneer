#!/usr/bin/env python3
"""Build isolated dynamic BFP4 returning leaf; no device or Work authority."""
import argparse
import hashlib
import os
from pathlib import Path
import subprocess

assert __debug__ and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
p=argparse.ArgumentParser(description=__doc__);p.add_argument('haiku_build',type=Path);p.add_argument('output',type=Path)
group=p.add_mutually_exclusive_group()
group.add_argument('--reuse-input',action='store_true',help='build the separate offboard input-reuse experiment')
group.add_argument('--reuse-input-checksum',action='store_true',help='build the separate on-card diagnostic checksum experiment')
group.add_argument('--bulk-load',action='store_true',help='build the separate offboard bulk-load experiment')
a=p.parse_args()
assert a.haiku_build.is_absolute() and a.haiku_build.is_dir()
assert a.output.is_absolute() and not a.output.exists() and not a.output.is_symlink()
a.output.mkdir(mode=0o700);here=Path(__file__).resolve().parent
tool=str(a.haiku_build/'cross-tools-riscv64/bin/riscv64-unknown-haiku-')
commands=[
 [tool+'g++','-std=c++11','-O2','-Wall','-Wextra','-Werror','-ffreestanding',
  '-fno-exceptions','-fno-rtti','-fno-stack-protector','-fno-pic','-fno-pie',
  '-ffunction-sections','-fdata-sections','-fno-unwind-tables','-fno-asynchronous-unwind-tables',
  '-msmall-data-limit=0','-mno-relax','-march=rv32i_zicsr','-mabi=ilp32','-fstack-usage',
  '-c',str(here/'blackhole_q38_bfp4_worker_leaf.cpp'),'-o',str(a.output/'program.o')],
 [tool+'as','-march=rv32i','-mabi=ilp32','-mno-relax',str(here/'blackhole_q38_bfp4_dram_band_start.S'),'-o',str(a.output/'start.o')],
 [tool+'ld','-m','elf32lriscv','--no-relax','--gc-sections','-T',str(here/'blackhole_q38_native_dram_band.ld'),
  str(a.output/'start.o'),str(a.output/'program.o'),'-Map',str(a.output/'program.map'),'-o',str(a.output/'program.elf')],
]
if a.reuse_input or a.reuse_input_checksum:commands[0].insert(1,'-DPIONEER_Q38_BFP4_INPUT_REUSE_EXPERIMENT=1')
if a.reuse_input_checksum:commands[0].insert(1,'-DPIONEER_Q38_BFP4_INPUT_REUSE_CHECKSUM_EXPERIMENT=1')
if a.bulk_load:commands[0].insert(1,'-DPIONEER_Q38_BFP4_BULK_LOAD_EXPERIMENT=1')
for section in ('boot','text'):
 commands.append([tool+'objcopy','-O','binary','-j','.'+section,str(a.output/'program.elf'),str(a.output/(section+'.bin'))])
for cmd in commands:subprocess.run(cmd,check=True,cwd=a.output)
(a.output/'compile-argv.txt').write_text('\n'.join('\t'.join(cmd) for cmd in commands)+'\n')
for cmd,name in (([tool+'nm','-n',str(a.output/'program.elf')],'program.symbols'),([tool+'objdump','-d',str(a.output/'program.elf')],'program.disassembly')):
 (a.output/name).write_bytes(subprocess.check_output(cmd))
for name in ('boot.bin','text.bin'):
 data=(a.output/name).read_bytes();assert len(data)==4 if name=='boot.bin' else 0<len(data)<=8192 and len(data)%4==0
 print(hashlib.sha256(data).hexdigest(),len(data),name)
print('BUILD ONLY dynamic BFP4 worker leaf; NO installation, device, Work, model quality or speed proof')
