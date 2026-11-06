#!/usr/bin/env python3
"""Build isolated K5120 arithmetic TRACE;not native authorization or a speed test."""
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys
assert __debug__ and len(sys.argv)==3 and not os.environ.get('PYTHONOPTIMIZE')
sdk,out=map(Path,sys.argv[1:]);here=Path(__file__).resolve().parent
assert sdk.is_absolute() and sdk.is_dir() and out.is_absolute() and not out.exists() and not out.is_symlink()
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
out.mkdir(mode=0o700);tool=str(sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku-')
stem='blackhole_q38_factored_q4_trace'
flags=['-std=c++17','-Os','-Wall','-Wextra','-Werror','-ffreestanding','-fno-builtin','-fno-tree-loop-distribute-patterns',
 '-fno-exceptions','-fno-rtti','-fno-stack-protector','-fno-pic','-fno-pie','-ffp-contract=off','-ffunction-sections',
 '-fdata-sections','-fno-unwind-tables','-fno-asynchronous-unwind-tables','-msmall-data-limit=0','-mno-relax',
 '-mabi=ilp32','-ffixed-s11','-fomit-frame-pointer','-fstack-usage','-DPIONEER_Q38_FACTORED_Q4_TRACE_BUILD=1']
commands=[
 [tool+'g++',*flags,'-DPIONEER_Q38_Q4_FACTORED_BFP8_BUILD=1','-march=rv32i','-c',str(here/(stem+'.cpp')),'-o',str(out/'program.o')],
 [tool+'as','-march=rv32i','-mabi=ilp32','-mno-relax',str(here/(stem+'_start.S')),'-o',str(out/'start.o')],
 [tool+'ld','-m','elf32lriscv','--no-relax','--gc-sections','-T',str(here/(stem+'.ld')),str(out/'start.o'),str(out/'program.o'),'-Map',str(out/'program.map'),'-o',str(out/'program.elf')]]
for section in ('boot','text'):commands.append([tool+'objcopy','-O','binary','-j','.'+section,str(out/'program.elf'),str(out/(section+'.bin'))])
for c in commands:subprocess.run(c,cwd=out,check=True)
assert not subprocess.check_output([tool+'nm','-u',str(out/'program.elf')]).strip()
symbols=subprocess.check_output([tool+'nm','-n','-S',str(out/'program.elf')],text=True)
assert not re.search(r'^[0-9a-f]+ [0-9a-f]+ [bBdDgGsS] ',symbols,re.M)
(out/'program.symbols').write_text(symbols)
(out/'program.disassembly').write_bytes(subprocess.check_output([tool+'objdump','-d',str(out/'program.elf')]))
(out/'build-argv.json').write_text(json.dumps(commands,indent=2)+'\n')
code=(out/'text.bin').read_bytes();assert 0<len(code)<=16384 and len(code)%4==0
assert all(int.from_bytes(code[off:off+4],'little')&127!=0x73 for off in range(0,len(code),4)), 'Trace has no CSR/clock capability'
for unit in ('program',):
 frames=[r.split('\t') for r in (out/(unit+'.su')).read_text().splitlines()]
 assert frames and all(len(r)==3 and r[2]=='static' and int(r[1])<=4096 for r in frames)
for name in ('boot.bin','text.bin'):
 data=(out/name).read_bytes();assert name!='boot.bin' or len(data)==4
 print('FACTORED_TRACE_BUILD',name,len(data),hashlib.sha256(data).hexdigest(),flush=True)
print('BUILD_ONLY_TRACE64_WORDS_NO_CSRS_16KiB_SCOPE_NOT_NATIVE_OWNER_OR_HARDWARE_SPEED')
