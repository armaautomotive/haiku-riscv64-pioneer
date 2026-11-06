#!/usr/bin/env python3
"""Build live BF16 projection/gated-FFN producer; OFFBOARD only."""
import hashlib
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
role='-DPIONEER_Q38_BF16_ACTIVATION_IMAGE=1';source=here/'blackhole_q38_bf16_activation.cpp'
flags=[role,'-std=c++17','-O2','-Wall','-Wextra','-Werror','-ffreestanding','-fno-builtin',
 '-fno-tree-loop-distribute-patterns','-fno-exceptions','-fno-rtti','-fno-stack-protector','-fno-pic','-fno-pie',
 '-ffp-contract=off','-ffunction-sections','-fdata-sections','-fno-unwind-tables','-fno-asynchronous-unwind-tables',
 '-msmall-data-limit=0','-mno-relax','-march=rv32imf','-mabi=ilp32','-ffixed-s11','-fstack-usage']
commands=[[tool+'g++',*flags,'-c',source,'-o',out/'program.o'],
 [tool+'as','-march=rv32imf','-mabi=ilp32','-mno-relax',here/'blackhole_q38_bf16_activation_start.S','-o',out/'start.o'],
 [tool+'ld','-m','elf32lriscv','--no-relax','--gc-sections','-T',here/'blackhole_q38_bf16_activation.ld',
  out/'start.o',out/'program.o','-Map',out/'program.map','-o',out/'program.elf']]
for section,leaf in (('boot','boot.bin'),('text','code.bin')):
 commands.append([tool+'objcopy','-O','binary','-j','.'+section,out/'program.elf',out/leaf])
commands.append([tool+'g++',*flags,'-DPIONEER_Q38_BF16_ACTIVATION_RETURN_ONLY=1','-c',source,'-o',out/'return-only.o'])
for command in commands:subprocess.run(list(map(str,command)),cwd=out,check=True)
assert set(row.split()[-1] for row in subprocess.check_output([tool+'nm','-u',str(out/'return-only.o')],text=True).splitlines())=={'bf16_activation_fault'}
assert not subprocess.check_output([tool+'nm','-u',str(out/'program.elf')]).strip()
symbols=subprocess.check_output([tool+'nm','-n',str(out/'program.elf')],text=True)
assert not re.search(r'^[0-9a-f]+ [bBdDgGsS] ',symbols,re.M)
dis=subprocess.check_output([tool+'objdump','-d',str(out/'program.elf')],text=True)
assert not re.search(r'\b(?:divu?|remu?|mulh[su]*|fdiv\.[sd]|fsqrt\.[sd]|fmadd|fmsub|fnmadd|fnmsub|fcvt\.wu?\.s|ebreak|s11)\b',dis)
assert not any(int(row,16)&127==0x73 for row in re.findall(r'^\s*[0-9a-f]+:\s+([0-9a-f]{8})\s',dis,re.M))
(out/'program.symbols').write_text(symbols);(out/'program.disassembly').write_text(dis)
frames=[row.split('\t') for row in (out/'program.su').read_text().splitlines()]
assert frames and all(len(row)==3 and row[2]=='static' and int(row[1])<=2048 for row in frames)
refused=[('-UPIONEER_Q38_BF16_ACTIVATION_IMAGE',),('-UPIONEER_Q38_BF16_ACTIVATION_IMAGE','-DPIONEER_Q38_BF16_ACTIVATION_IMAGE=0'),('-ffast-math',)]
for name in ('_KERNEL_MODE','PIONEER_Q38_FULL_OFFLOAD_NATIVE','PIONEER_Q38_FULL_OFFLOAD_CLIENT','PIONEER_DRIVER_BUILD_TAG',
 'PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE','PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_FULL_MODEL_CPU_REFERENCE',
 'PIONEER_Q38_PRECISE_ACTIVATION_CHUNK_IMAGE'):
 for value in (0,1):refused.append(('-D'+name+'='+str(value),))
for extra in refused:
 result=subprocess.run([tool+'g++',*flags,*extra,'-E',str(source)],stdout=subprocess.DEVNULL,stderr=subprocess.PIPE)
 assert result.returncode and b'error:' in result.stderr,extra
(out/'role-refusals.txt').write_text(''.join('PASS '+' '.join(extra)+'\n' for extra in refused))
for leaf in ('boot.bin','code.bin','program.elf'):
 data=(out/leaf).read_bytes();assert data and not len(data)%4
 print('BF16_ACTIVATION_BUILD',leaf,len(data),hashlib.sha256(data).hexdigest(),flush=True)
print('PASS BUILD_ONLY live BF16 producer projection/precise-gated-FFN,no Q8 quantizer or scale;return-only/s11 fixed/roles'+str(len(refused))+';NO native/worker/ROOT/hardware/model-quality/TPS authority')
