#!/usr/bin/env python3
"""Compile NEW local27B Q/K preparation ONLY. No native build or device IO."""
import hashlib
import os
import pathlib
import re
import subprocess
import sys
if len(sys.argv)!=3 or any(k.startswith('PIONEER_') for k in os.environ):
    raise SystemExit(2)
sdk,out=map(pathlib.Path,sys.argv[1:])
assert sdk.is_absolute() and out.is_absolute() and not out.exists() and not out.is_symlink()
here=pathlib.Path(__file__).resolve().parent
cross=sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku'
assert pathlib.Path(str(cross)+'-g++').is_file()
out.mkdir(mode=0o700)
def run(*args):
    subprocess.run([str(a) for a in args],check=True)
run(str(cross)+'-g++','-std=c++11','-O2','-Wall','-Wextra','-Werror','-ffreestanding','-fno-exceptions',
    '-fno-rtti','-fno-stack-protector','-fno-pic','-fno-pie','-ffp-contract=off','-ffunction-sections','-fdata-sections',
    '-fno-unwind-tables','-fno-asynchronous-unwind-tables','-msmall-data-limit=0','-mno-relax','-march=rv32imf','-mabi=ilp32',
    '-fstack-usage','-DPIONEER_Q38_DENSE_PREPARE_IMAGE=1','-c',here/'blackhole_qwen38_dense_prepare.cpp','-o',out/'program.o')
run(str(cross)+'-as','-march=rv32imf','-mabi=ilp32','-mno-relax',here/'blackhole_qwen38_dense_prepare_start.S','-o',out/'start.o')
run(str(cross)+'-ld','-m','elf32lriscv','--no-relax','--gc-sections','-T',here/'blackhole_scalar.ld',
    out/'start.o',out/'program.o','-Map',out/'program.map','-o',out/'program.elf')
for section,name in (('text','code'),('boot','boot')):
    run(str(cross)+'-objcopy','-O','binary','-j','.'+section,out/'program.elf',out/(name+'.bin'))
for tool,args,name in (('nm',['-n'],'symbols'),('nm',['-u'],'undefined'),('objdump',['-d'],'disassembly'),('objdump',['-h'],'sections')):
    with (out/('program.'+name)).open('w') as target:
        subprocess.run([str(cross)+'-'+tool,*args,str(out/'program.elf')],stdout=target,check=True)
assert not (out/'program.undefined').read_bytes()
symbols=(out/'program.symbols').read_text();dis=(out/'program.disassembly').read_text()
assert not re.search(r'^[0-9a-f]+ [bBdDgGsS] ',symbols,re.M)
assert not re.search(r'\b(?:divu?|remu?|mulh[su]*|fdiv\.[sd]|fsqrt\.[sd]|fmadd|fmsub|fnmadd|fnmsub|fcvt\.wu?\.s|ebreak)\b',dis)
for line in dis.splitlines():
    m=re.match(r'\s*[0-9a-f]+:\s+([0-9a-f]{8})\s',line)
    if m: assert int(m[1],16)&127!=0x73,'NO CSR instructions in local math'
for name,address in (('scalar_entry',0x64000),('scalar_done',0x6400c),('scalar_park',0x64038),('scalar_fault_park',0x6403c)):
    assert re.search('^'+format(address,'08x')+' T '+name+'$',symbols,re.M),name
frames=[int(line.split('\t')[1]) for line in (out/'program.su').read_text().splitlines()]
assert frames and max(frames)<=2048
for name in ('boot.bin','code.bin','program.elf'):
    data=(out/name).read_bytes();print(hashlib.sha256(data).hexdigest()+' '+name+' bytes'+str(len(data)))
print('PASS COMPILED NEW local27B sixQ+oneK/V preparation/maxFrame'+str(max(frames))+';NO transport/native/KV/token loop/speed grant')
