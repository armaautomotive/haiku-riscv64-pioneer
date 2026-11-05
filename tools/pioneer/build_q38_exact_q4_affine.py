#!/usr/bin/env python3
"""Compile distinct OFFBOARD full-K original-Q4 affine;NO driver/installation/GPU."""
import argparse,hashlib,os,pathlib,subprocess,sys
if sys.flags.optimize:raise SystemExit('REFUSE optimized Python')
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('sdk',type=pathlib.Path);p.add_argument('output',type=pathlib.Path);p.add_argument('width',type=int,choices=(5120,6144,17408))
p.add_argument('--fixed-fixture',action='store_true',help='distinct fixed synthetic17408 qualification image,not packet/model data')
a=p.parse_args()
assert not a.fixed_fixture or a.width==17408
assert a.sdk.is_absolute() and a.sdk.is_dir() and a.output.is_absolute() and not a.output.exists() and not a.output.is_symlink()
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
here=pathlib.Path(__file__).resolve().parent;tool=str(a.sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku-');a.output.mkdir(mode=0o700)
commands=[
 [tool+'g++','-std=c++11','-O2','-Wall','-Wextra','-Werror','-ffreestanding','-fno-exceptions','-fno-rtti','-fno-stack-protector','-fno-pic','-fno-pie',
  '-ffunction-sections','-fdata-sections','-fno-unwind-tables','-fno-asynchronous-unwind-tables','-msmall-data-limit=0','-mno-relax','-march=rv32i','-mabi=ilp32','-fstack-usage',
  '-DPIONEER_EXACT_Q4_AFFINE_K='+str(a.width),'-c',str(here/'blackhole_q38_exact_q4_affine.cpp'),'-o',str(a.output/'program.o')],
 [tool+'as','-march=rv32i','-mabi=ilp32','-mno-relax',str(here/'blackhole_q38_exact_q4_affine_start.S'),'-o',str(a.output/'start.o')],
 [tool+'ld','-m','elf32lriscv','--no-relax','--gc-sections','-T',str(here/'blackhole_q38_exact_q4_affine.ld'),str(a.output/'start.o'),str(a.output/'program.o'),'-Map',str(a.output/'program.map'),'-o',str(a.output/'program.elf')]]
if a.fixed_fixture:commands[0][1:1]=['-DPIONEER_EXACT_Q4_AFFINE_FIXED=1']
for section in ('boot','text'):commands.append([tool+'objcopy','-O','binary','-j','.'+section,str(a.output/'program.elf'),str(a.output/(section+'.bin'))])
for command in commands:subprocess.run(command,check=True,cwd=a.output)
(a.output/'compile-argv.txt').write_text('\n'.join('\t'.join(c) for c in commands)+'\n')
for suffix,name in (('nm','program.symbols'),('objdump','program.disassembly')):
 with (a.output/name).open('w') as output:subprocess.run([tool+suffix,'-n' if suffix=='nm' else '-d',str(a.output/'program.elf')],stdout=output,check=True)
for name in ('boot.bin','text.bin'):
 data=(a.output/name).read_bytes();assert len(data)==4 if name=='boot.bin' else 0<len(data)<=8192 and len(data)%4==0
 print(hashlib.sha256(data).hexdigest(),len(data),name)
print('OFFBOARD_ONLY '+('FIXED_SYNTHETIC_' if a.fixed_fixture else '')+'fullK'+str(a.width)+' ORIGINAL_PER_K16_AFFINE;NO native,model,hardware or TPS authority')
