#!/usr/bin/env python3
"""Build isolated old/new comparison,not native authorization or timing proof."""
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys
configured=sys.argv[-1:]==['--configured']
if configured:sys.argv.pop()
assert __debug__ and len(sys.argv)==3 and not os.environ.get('PYTHONOPTIMIZE')
sdk,out=map(Path,sys.argv[1:]);here=Path(__file__).resolve().parent
assert sdk.is_absolute() and sdk.is_dir() and out.is_absolute() and not out.exists() and not out.is_symlink()
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
out.mkdir(mode=0o700);tool=str(sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku-')
stem='blackhole_q38_factored_q4_compare'
flags=['-std=c++17','-Os','-Wall','-Wextra','-Werror','-ffreestanding','-fno-builtin','-fno-tree-loop-distribute-patterns',
 '-fno-exceptions','-fno-rtti','-fno-stack-protector','-fno-pic','-fno-pie','-ffp-contract=off','-ffunction-sections',
 '-fdata-sections','-fno-unwind-tables','-fno-asynchronous-unwind-tables','-msmall-data-limit=0','-mno-relax',
 '-mabi=ilp32','-ffixed-s11','-fomit-frame-pointer','-fstack-usage','-DPIONEER_Q38_FACTORED_Q4_COMPARE_BUILD=1']
if configured:flags+=['-DPIONEER_Q38_FACTORED_CONFIGURED_COMPUTE=1']
commands=[
 [tool+'g++',*flags,'-DPIONEER_Q38_Q4_FACTORED_BFP8_BUILD=1','-march=rv32i','-c',str(here/(stem+'.cpp')),'-o',str(out/'program.o')],
 [tool+'g++',*flags,'-march=rv32i_zicsr','-c',str(here/'blackhole_q38_q4_kernel_clock.cpp'),'-o',str(out/'clock.o')],
 [tool+'as','-march=rv32i','-mabi=ilp32','-mno-relax',str(here/(stem+'_start.S')),'-o',str(out/'start.o')],
 [tool+'ld','-m','elf32lriscv','--no-relax','--gc-sections','-T',str(here/(stem+'.ld')),str(out/'start.o'),str(out/'program.o'),str(out/'clock.o'),'-Map',str(out/'program.map'),'-o',str(out/'program.elf')]]
for section in ('boot','text'):commands.append([tool+'objcopy','-O','binary','-j','.'+section,str(out/'program.elf'),str(out/(section+'.bin'))])
for c in commands:subprocess.run(c,cwd=out,check=True)
assert not subprocess.check_output([tool+'nm','-u',str(out/'program.elf')]).strip()
symbols=subprocess.check_output([tool+'nm','-n','-S',str(out/'program.elf')],text=True)
assert not re.search(r'^[0-9a-f]+ [0-9a-f]+ [bBdDgGsS] ',symbols,re.M)
(out/'program.symbols').write_text(symbols)
(out/'program.disassembly').write_bytes(subprocess.check_output([tool+'objdump','-d',str(out/'program.elf')]))
(out/'build-argv.json').write_text(json.dumps(commands,indent=2)+'\n')
code=(out/'text.bin').read_bytes();assert 0<len(code)<=16384 and len(code)%4==0
clock=re.search(r'^([0-9a-f]+) ([0-9a-f]+) T q38_q4_kernel_clock$',symbols,re.M);assert clock
start,size=(int(v,16) for v in clock.groups());csrs=[]
for off in range(0,len(code),4):
 w=int.from_bytes(code[off:off+4],'little')
 if w&127==0x73:
  assert start<=0x70000+off<start+size and w&0x000ff07f==0x2073 and (w>>7)&31
  csrs.append((off,w>>20))
assert len(csrs)==3 and [c for _,c in csrs]==[0xb80,0xb00,0xb80] and csrs[2][0]==csrs[1][0]+4==csrs[0][0]+8
for unit in ('program','clock'):
 frames=[r.split('\t') for r in (out/(unit+'.su')).read_text().splitlines()]
 assert frames and all(len(r)==3 and r[2]=='static' and int(r[1])<=4096 for r in frames)
for name in ('boot.bin','text.bin'):
 data=(out/name).read_bytes();assert name!='boot.bin' or len(data)==4
 print('FACTORED_COMPARE_BUILD',name,len(data),hashlib.sha256(data).hexdigest(),flush=True)
print('BUILD_ONLY_NEW16KiB_SCOPE_threeReadOnlyClockCSRs_NOT_NATIVE_OWNER_OR_HARDWARE_SPEED')
