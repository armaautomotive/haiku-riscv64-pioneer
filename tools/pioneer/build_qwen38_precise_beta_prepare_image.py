#!/usr/bin/env python3
"""Actual combined-input preparation math with precise beta; LOCAL TEST ONLY."""
import hashlib
import os
import pathlib
import re
import subprocess
import sys

inspect='--inspect-candidate' in sys.argv
if inspect:
    assert sys.argv.count('--inspect-candidate')==1;sys.argv.remove('--inspect-candidate')
assert len(sys.argv)==3 and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
sdk,out=map(pathlib.Path,sys.argv[1:]);here=pathlib.Path(__file__).resolve().parent
assert sdk.is_absolute() and out.is_absolute() and not out.exists() and not out.is_symlink()
cross=str(sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku')
assert pathlib.Path(cross+'-g++').is_file();out.mkdir(mode=0o700)
def run(*args):subprocess.run(list(map(str,args)),check=True)
run(cross+'-g++','-std=c++11','-O2','-Wall','-Wextra','-Werror','-ffreestanding','-fno-exceptions','-fno-rtti',
    '-fno-stack-protector','-fno-pic','-fno-pie','-ffp-contract=off','-ffunction-sections','-fdata-sections',
    '-fno-unwind-tables','-fno-asynchronous-unwind-tables','-msmall-data-limit=0','-mno-relax','-march=rv32imf','-mabi=ilp32',
    '-DPIONEER_Q38_GDN_INPUT_IMAGE=1','-DPIONEER_Q38_GDN_INPUT_COMBINED_STAGE=1',
    '-DPIONEER_Q38_GDN_PRECISE_BETA_IMAGE=1','-DPIONEER_Q38_SIGMOID_PRECISION_IMAGE=1',
    '-fstack-usage','-c',here/'blackhole_qwen38_gdn_prepare.cpp','-o',out/'program.o')
run(cross+'-as','-march=rv32imf','-mabi=ilp32','-mno-relax',here/'blackhole_qwen38_gdn_prepare_start.S','-o',out/'start.o')
run(cross+'-ld','-m','elf32lriscv','--no-relax','--gc-sections','-T',here/'blackhole_scalar.ld',out/'start.o',out/'program.o',
    '-Map',out/'program.map','-o',out/'program.elf')
for section,name in (('text','code'),('boot','boot')):run(cross+'-objcopy','-O','binary','-j','.'+section,out/'program.elf',out/(name+'.bin'))
symbols=subprocess.check_output([cross+'-nm','-n',str(out/'program.elf')],text=True)
assert not subprocess.check_output([cross+'-nm','-u',str(out/'program.elf')],text=True).strip()
assert not re.search(r'^[0-9a-f]+ [bBdDgGsS] ',symbols,re.M)
dis=subprocess.check_output([cross+'-objdump','-d',str(out/'program.elf')],text=True)
assert not re.search(r'\b(?:divu?|remu?|mulh[su]*|fdiv\.[sd]|fsqrt\.[sd]|fmadd|fmsub|fnmadd|fnmsub|fcvt\.wu?\.s|ebreak)\b',dis)
for line in dis.splitlines():
    match=re.match(r'\s*[0-9a-f]+:\s+([0-9a-f]{8})\s',line)
    if match:assert int(match[1],16)&127!=0x73,'No CSR instructions in local math probe'
for name,address in (('scalar_entry',0x64000),('scalar_park',0x64038),('scalar_fault_park',0x6403c)):
    assert re.search('^'+format(address,'08x')+' T '+name+'$',symbols,re.M),name
frames=[int(line.split('\t')[1]) for line in (out/'program.su').read_text().splitlines()]
assert frames and max(frames)<=512
(out/'program.symbols').write_text(symbols);(out/'program.disassembly').write_text(dis)
observed=[]
for name in ('boot.bin','code.bin','program.elf'):
    data=(out/name).read_bytes();assert data and not len(data)%4
    observed.append((name,len(data),hashlib.sha256(data).hexdigest()));print(observed[-1])
PINS=(
    ('boot.bin',4,'d0565e4ef0f59aa08ad0be91ffae61ceb85aedff5f36f346c95312b0f9407ac6'),
    ('code.bin',6476,'2cec5069905e82faf1d42c2263c7206ea8fcef708cf172ee2fd0f18e55e627d4'),
    ('program.elf',15932,'9fcb223a34689cbca39decfb875e6d957e3b2d13d37d260e63cc0268c89c77ac'),
) # Independent main/clean BUILD artifacts, not runtime/native proof.
if not inspect:assert PINS and tuple(observed)==PINS,'Unqualified local probe: inspect/reproduce/freeze first'
print('BUILD_ONLY actual combined GDN preparation+precise beta/maxIndividualFrame'+str(max(frames))+
      ';LOCAL_DATA_ONLY_NOT_WORKER_NATIVE_PHYSICAL_WORK_OR_TPS;DO_NOT_INSTALL')
