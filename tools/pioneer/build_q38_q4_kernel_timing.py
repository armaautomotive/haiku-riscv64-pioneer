#!/usr/bin/env python3
"""Inspect-only timed Q4 kernel BUILD. No native owner or hardware authority."""
import argparse
import hashlib
import os
from pathlib import Path
import re
import runpy
import subprocess
import sys

INSPECT_PINS={
    5120:('8156dd4e49912270887cf45f98e9b7c2892a0549a72c909dbefc502aa2205512',
          'b53df880ebdfb803b1a6f4c19ff4a4e7b30395aad6771b81007a4ab586e6019f'),
    17408:('9723a83e4550cd66e36613f49d0959251ac0d5f755b903568e8725d4a345ac3f',
           'b04587b1df60e7527cfb26400f71aae44495eb9f00f89ca791ba09e860f5980f'),
} # Inspect identities,NOT execution/owner grants.

def audit_clock(code,clock_start,clock_size):
    assert len(code)==7972 and clock_size>0
    csrs=[]
    for offset in range(0,len(code),4):
        word=int.from_bytes(code[offset:offset+4],'little')
        if word&0x7f!=0x73:
            continue
        pc=0x70000+offset
        assert clock_start<=pc<clock_start+clock_size
        assert word&0x000ff07f==0x2073 and (word>>7)&31
        csrs.append((pc,word>>20))
    assert [csr for _,csr in csrs]==[0xb80,0xb00,0xb80]
    assert csrs[1][0]==csrs[0][0]+4 and csrs[2][0]==csrs[0][0]+8
    return csrs

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('sdk',type=Path)
    parser.add_argument('out',type=Path)
    parser.add_argument('width',type=int,choices=(5120,17408))
    args=parser.parse_args()
    if not __debug__ or os.environ.get('PYTHONOPTIMIZE'):
        parser.error('checks required')
    assert args.sdk.is_absolute() and args.sdk.is_dir()
    assert args.out.is_absolute() and not args.out.exists() and not args.out.is_symlink()
    assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
    here=Path(__file__).resolve().parent
    args.out.mkdir(mode=0o700)
    control=args.out/'unchanged-compact'
    sys.argv=[str(here/'build_q38_q4_fpu_compact_stream.py'),str(args.sdk),str(control),str(args.width),'--inspect-candidate']
    old=runpy.run_path(sys.argv[0]) # First reproduce BOTH original images.
    commands=[[arg.replace(str(control),str(args.out)) for arg in row] for row in old['commands']]
    commands[0].insert(1,'-DPIONEER_Q38_COMPACT_Q4_TIMING_IMAGE=1')
    commands[1][1:1]=['--defsym','PIONEER_Q38_COMPACT_Q4_TIMING_IMAGE=1']
    clock=[arg.replace('blackhole_q38_q4_fpu_compact_stream.cpp','blackhole_q38_q4_kernel_clock.cpp')
           .replace('program.o','clock.o').replace('-march=rv32i','-march=rv32i_zicsr') for arg in commands[0]]
    commands.insert(1,clock)
    link=next(row for row in commands if row[0].endswith('ld'))
    link.insert(link.index('-Map'),str(args.out/'clock.o'))
    for command in commands:
        subprocess.run(command,check=True,cwd=args.out)
    tool=str(args.sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku-')
    elf=args.out/'program.elf'
    assert not subprocess.check_output([tool+'nm','-u',str(elf)]).strip()
    symbols=subprocess.check_output([tool+'nm','-n','-S',str(elf)],text=True)
    assert not re.search(r'^[0-9a-f]+ [0-9a-f]+ [bBdDgGsS] ',symbols,re.M)
    (args.out/'program.symbols').write_text(symbols)
    (args.out/'program.disassembly').write_bytes(subprocess.check_output([tool+'objdump','-d',str(elf)]))
    (args.out/'compile-argv.txt').write_text('\n'.join('\t'.join(row) for row in commands)+'\n')
    clock_symbol=re.search(r'^([0-9a-f]+) ([0-9a-f]+) T q38_q4_kernel_clock$',symbols,re.M)
    assert clock_symbol
    clock_start,clock_size=(int(value,16) for value in clock_symbol.groups())
    code=(args.out/'text.bin').read_bytes()
    assert len(code)==7972 and hashlib.sha256(code).hexdigest()==INSPECT_PINS[args.width][0]
    assert elf.stat().st_size==20300 and hashlib.sha256(elf.read_bytes()).hexdigest()==INSPECT_PINS[args.width][1]
    audit_clock(code,clock_start,clock_size)
    for unit in ('program','clock'):
        frames=[line.split('\t') for line in (args.out/(unit+'.su')).read_text().splitlines()]
        assert frames and all(len(row)==3 and row[2]=='static' and int(row[1])<=4096 for row in frames)
    for name in ('boot.bin','text.bin','program.elf'):
        data=(args.out/name).read_bytes()
        print('Q4_KERNEL_TIMING_INSPECT',args.width,name,len(data),hashlib.sha256(data).hexdigest(),flush=True)
    print('BUILD_ONLY distinct QCT1/three readonly CSRRS in bounded sampler/8KiB ROM;NO_NATIVE_OWNER_OR_HARDWARE_TPS',flush=True)

if __name__=='__main__':main()
