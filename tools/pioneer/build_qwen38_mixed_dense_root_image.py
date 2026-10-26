#!/usr/bin/env python3
"""NEW persistent QMR1 controller ONLY; no native build/install/card IO."""
import hashlib
import os
import pathlib
import re
import subprocess
import sys
if len(sys.argv) != 3 or any(k.startswith('PIONEER_') for k in os.environ):
    raise SystemExit(2)
sdk, out = map(pathlib.Path, sys.argv[1:])
assert sdk.is_absolute() and out.is_absolute() and not out.exists() and not out.is_symlink()
here = pathlib.Path(__file__).resolve().parent
cross = sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku'
assert pathlib.Path(str(cross)+'-g++').is_file()
out.mkdir(mode=0o700)
def run(*args):
    subprocess.run([str(a) for a in args], check=True)
run(sys.executable, here/'build_qwen38_dense_partition_root_image.py', sdk, out/'unchanged-root')
for name, size, digest in (
    ('boot.bin',4,'49beea42a964dc73343fca928d59be0eb6a6d28e4c1354b47894f3f60ef4a31a'),
    ('code.bin',12140,'a8aa43b88af5d821a50498d0e79a75025ca672490fa6d4ead51be8442013bf8c'),
    ('program.elf',21528,'213afbc1746516e470fe8e9614a2723f3a0e4a00c8dfc3d562513d9cf69d36af')):
    data = (out/'unchanged-root'/name).read_bytes()
    assert len(data) == size and hashlib.sha256(data).hexdigest() == digest, name
run(str(cross)+'-g++', '-std=c++11', '-O2', '-Wall', '-Wextra', '-Werror', '-ffreestanding', '-fno-exceptions',
    '-fno-rtti', '-fno-stack-protector', '-fno-pic', '-fno-pie', '-ffp-contract=off', '-ffunction-sections', '-fdata-sections',
    '-fno-unwind-tables', '-fno-asynchronous-unwind-tables', '-msmall-data-limit=0', '-mno-relax', '-march=rv32imf', '-mabi=ilp32',
    '-fstack-usage', '-ffixed-s11', '-DPIONEER_Q38_MIXED_DENSE_ROOT_IMAGE=1', '-c', here/'blackhole_qwen38_mixed_dense_root.cpp', '-o', out/'program.o')
run(str(cross)+'-as', '-march=rv32imf', '-mabi=ilp32', '-mno-relax', here/'blackhole_qwen38_mixed_dense_root_start.S', '-o', out/'start.o')
run(str(cross)+'-ld', '-m', 'elf32lriscv', '--no-relax', '--gc-sections', '-T', here/'blackhole_qwen38_dense_partition_root.ld',
    out/'start.o', out/'program.o', '-Map', out/'program.map', '-o', out/'program.elf')
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
    if m: assert int(m[1],16)&127!=0x73, 'NO CSR instructions in controller'
reserved=[line for line in dis.splitlines() if re.search(r'\bs11\b',line)]
assert len(reserved)==2 and sum(bool(re.search(r'\bmv\s+s11,\w+\s*$',line)) for line in reserved)==1
assert sum(bool(re.search(r'\bmv\s+\w+,s11\s*$',line)) for line in reserved)==1
for name,address in (('scalar_entry',0x50000),('mixed_root_wait_load',0x5004c),('mixed_root_wait_branch',0x50050),('scalar_fault_park',0x50080),('mixed_root_fault_stopped',0x5009c)):
    assert re.search('^'+format(address,'08x')+' T '+name+'$',symbols,re.M),name
frames=[int(line.split('\t')[1]) for line in (out/'program.su').read_text().splitlines()]
assert frames and max(frames)<=2048
rows=(out/'program.sections').read_text().splitlines()
for i,row in enumerate(rows):
    m=re.match(r'\s*\d+\s+(\S+)\s+([0-9a-fA-F]+)\s+',row)
    if m and 'ALLOC' in rows[i+1]: assert m[1] in ('.boot','.text'),row
for name in ('boot.bin','code.bin','program.elf'):
    data=(out/name).read_bytes();print(hashlib.sha256(data).hexdigest()+' '+name+' bytes'+str(len(data)))
print('PASS NEW persistent QMR1 ROOT/all119 live admission/dynamic QMW1 nonces/96 returning dense workers/global max+sum+inverse+merge/no worker boots or STOP/maxFrame'+str(max(frames))+';native/full-model/original producer/speed still OWED')
