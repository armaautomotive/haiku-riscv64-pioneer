#!/usr/bin/env python3
"""NEW dense QG/K/V/Q6-O matrix root. Compile ONLY,not native/device IO."""
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
here=pathlib.Path(__file__).resolve().parent;cross=sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku'
assert pathlib.Path(str(cross)+'-g++').is_file();out.mkdir(mode=0o700)
def run(*args):
    subprocess.run([str(a) for a in args],check=True)
flags=['PIONEER_Q38_DENSE_PROJECTION_IMAGE=1','PIONEER_Q38_PROJECTION_MATRIX_ROOT=1','PIONEER_Q38_FFN_RESIDENT_MATRIX',
       'PIONEER_Q38_COMPACT_ACTIVATIONS','PIONEER_Q38_REUSE_ACTIVATIONS','PIONEER_Q38_WORK_COHORT_CACHE',
       'PIONEER_Q38_WORK_SKIP_DOT_CHECK','PIONEER_Q38_WORK_SPARSE_EXPORT','PIONEER_Q38_WORK_COMPACT_OUTPUT',
       'PIONEER_Q38_ROOT_CYCLE_PROFILE','PIONEER_Q38_WORK_SENTINEL_POLL']
run(str(cross)+'-g++','-std=c++11','-O2','-Wall','-Wextra','-Werror','-ffreestanding','-fno-exceptions','-fno-rtti',
    '-fno-stack-protector','-fno-pic','-fno-pie','-ffp-contract=off','-ffunction-sections','-fdata-sections','-fno-unwind-tables',
    '-fno-asynchronous-unwind-tables','-msmall-data-limit=0','-mno-relax','-march=rv32imf','-mabi=ilp32','-fstack-usage','-fomit-frame-pointer',
    *['-D'+f for f in flags],'-c',here/'blackhole_qwen38_dense_projection.cpp','-o',out/'program.o')
run(str(cross)+'-as','-march=rv32imf','-mabi=ilp32','-mno-relax','--defsym','TT_Q38_DISPATCH_ROOT=1',
    '--defsym','TT_Q38_FFN_MATRIX_ROOT=1','--defsym','TT_Q38_PROJECTION_MATRIX_ROOT=1',
    here/'blackhole_qwen38_dense_projection_start.S','-o',out/'start.o')
run(str(cross)+'-ld','-m','elf32lriscv','--no-relax','--gc-sections','-T',here/'blackhole_qwen38_ffn_matrix.ld',
    out/'start.o',out/'program.o','-Map',out/'program.map','-o',out/'program.elf')
for name in ('boot','text','prefix'):run(str(cross)+'-objcopy','-O','binary','-j','.'+name,out/'program.elf',out/(name+'.bin'))
for tool,args,name in (('nm',['-n'],'symbols'),('nm',['-u'],'undefined'),('objdump',['-d'],'disassembly'),('objdump',['-h'],'sections')):
    with (out/('program.'+name)).open('w') as target:subprocess.run([str(cross)+'-'+tool,*args,str(out/'program.elf')],stdout=target,check=True)
assert not (out/'program.undefined').read_bytes()
symbols=(out/'program.symbols').read_text();dis=(out/'program.disassembly').read_text()
assert not re.search(r'^[0-9a-f]+ [bBdDgGsS] ',symbols,re.M)
assert not re.search(r'\b(?:divu?|remu?|mulh[su]*|fdiv\.[sd]|fsqrt\.[sd]|fmadd|fmsub|fnmadd|fnmsub|fcvt\.wu?\.s|ebreak)\b',dis)
csrs=[]
for line in dis.splitlines():
    m=re.match(r'\s*([0-9a-f]+):\s+([0-9a-f]{8})\s',line)
    if m and int(m[2],16)&127==0x73:
        word=int(m[2],16);assert (word>>12)&7==2 and (word>>15)&31==0 and (word>>7)&31 and word>>20 in (0xb00,0xb80)
        csrs.append(word>>20)
assert csrs==[0xb80,0xb00,0xb80]
for name,address in (('block_entry',0x72000),('block_done',0x7200c),('block_park',0x72038),('block_fault_park',0x72060)):
    assert re.search('^'+format(address,'08x')+' T '+name+'$',symbols,re.M),name
frames={line.split('\t')[0]:int(line.split('\t')[1]) for line in (out/'program.su').read_text().splitlines()}
assert frames and all(n<=2048 or name.endswith(':unsigned int block_main()') and n<=5120 for name,n in frames.items())
for name in ('boot.bin','text.bin','prefix.bin','program.elf'):
    data=(out/name).read_bytes();print(hashlib.sha256(data).hexdigest()+' '+name+' bytes'+str(len(data)))
print('PASS COMPILED newQDM1 canonical denseQG12288/K1024/V1024/Q6-O5120 waveROOT;maxFrame'+str(max(frames.values()))+';NO native/inputproducer/FAST Work/speed grant')
