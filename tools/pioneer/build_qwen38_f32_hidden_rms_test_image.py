#!/usr/bin/env python3
"""Separate bounded FP32 arithmetic TEST image. NO controller/native launch."""
import hashlib
import os
import pathlib
import re
import subprocess
import sys
assert len(sys.argv) in (3,4) and (len(sys.argv)==3 or sys.argv[3]=='--inspect-candidate')
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
sdk,out=map(pathlib.Path,sys.argv[1:3]);here=pathlib.Path(__file__).resolve().parent
assert sdk.is_absolute() and out.is_absolute() and not out.exists() and not out.is_symlink()
tool=str(sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku-');out.mkdir(mode=0o700)
def run(*args):subprocess.run([str(a) for a in args],check=True)
flags=['-std=c++11','-O2','-Wall','-Wextra','-Werror','-ffreestanding','-fno-exceptions','-fno-rtti','-fno-stack-protector',
 '-fno-pic','-fno-pie','-ffp-contract=off','-ffunction-sections','-fdata-sections','-fno-unwind-tables','-fno-asynchronous-unwind-tables',
 '-msmall-data-limit=0','-mno-relax','-march=rv32imf','-mabi=ilp32','-fstack-usage']
run(tool+'g++',*flags,'-DPIONEER_Q38_F32_HIDDEN_RMS_TEST_IMAGE=1','-c',here/'blackhole_qwen38_f32_hidden_rms_test.cpp','-o',out/'program.o')
run(tool+'as','-I'+str(here),'-march=rv32imf','-mabi=ilp32','-mno-relax',here/'blackhole_qwen38_learned_rms_start.S','-o',out/'start.o')
run(tool+'ld','-m','elf32lriscv','--no-relax','--gc-sections','-T',here/'blackhole_scalar.ld',out/'start.o',out/'program.o','-Map',out/'program.map','-o',out/'program.elf')
for section in ('text','boot'):run(tool+'objcopy','-O','binary','-j','.'+section,out/'program.elf',out/('code.bin' if section=='text' else 'boot.bin'))
symbols=subprocess.check_output([tool+'nm','-n',str(out/'program.elf')],text=True)
dis=subprocess.check_output([tool+'objdump','-d',str(out/'program.elf')],text=True)
assert not subprocess.check_output([tool+'nm','-u',str(out/'program.elf')],text=True).strip()
assert not re.search(r'^[0-9a-f]+ [bBdDgGsS] ',symbols,re.M)
assert not re.search(r'\b(?:divu?|remu?|mulh[su]*|fdiv\.[sd]|fsqrt\.[sd]|fmadd|fmsub|fnmadd|fnmsub|fcvt\.wu?\.s|ebreak)\b',dis)
for line in dis.splitlines():
    m=re.match(r'\s*[0-9a-f]+:\s+([0-9a-f]{8})\s',line)
    if m:assert int(m[1],16)&127!=0x73,'no controller CSR'
frames=[int(line.split('\t')[1]) for line in (out/'program.su').read_text().splitlines()]
assert frames and max(frames)<=1024
(out/'program.symbols').write_text(symbols);(out/'program.disassembly').write_text(dis)
PINS=(('code.bin',5084,'93da3bbe65a993ec702a52db7b8ad09a62c64f73dbf4e35e4d64379b74381d49'),
 ('boot.bin',4,'d0565e4ef0f59aa08ad0be91ffae61ceb85aedff5f36f346c95312b0f9407ac6'),
 ('program.elf',14416,'e893629f213f0d8b9f5d257339b2e8019494240da4f8a29f00f38c351e45ce41'))
observed=[]
for name in ('code.bin','boot.bin','program.elf'):
    data=(out/name).read_bytes();item=(name,len(data),hashlib.sha256(data).hexdigest());observed.append(item);print(repr(item),flush=True)
assert 0<len((out/'code.bin').read_bytes())<=32768 and len((out/'boot.bin').read_bytes())==4
if len(sys.argv)==3:assert PINS and tuple(observed)==PINS,'unqualified NUMERIC TEST image'
print('BUILD ONLY separate FP32 hidden RMS numeric TEST/maxFrame'+str(max(frames))+';NO full controller/native/physical/Work/launch/combined-stack or TPS grant')
