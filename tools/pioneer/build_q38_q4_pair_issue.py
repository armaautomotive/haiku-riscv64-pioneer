#!/usr/bin/env python3
"""Inspect-only paired Q4 issue BUILD; never installs or opens a device."""
import argparse
import hashlib
import os
from pathlib import Path
import re
import runpy
import subprocess
import sys

INSPECT_PINS={
    5120:('04b846ca7cb1fcbf0457abfa1790fda9ee68662d58215e76af5d3984bffee261',
          '6793281923b9c159c1c6e3a57fdf37e56ceb5738ea888de59c126d71a7645949'),
    17408:('1fc0e0076948d68a8c8112de23145162bca6de9a30a8f2b7025d3b1f6465d4da',
           '3d2381a78918700bed5e11254f5e62176f2458614106b0fc9ba10830f57519ac'),
} # Inspect identities, not native execution/ownership grants.


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('sdk',type=Path)
    parser.add_argument('out',type=Path)
    parser.add_argument('width',type=int,choices=(5120,17408))
    args=parser.parse_args()
    assert __debug__ and not os.environ.get('PYTHONOPTIMIZE')
    assert args.sdk.is_absolute() and args.sdk.is_dir()
    assert args.out.is_absolute() and not args.out.exists() and not args.out.is_symlink()
    assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
    here=Path(__file__).resolve().parent
    args.out.mkdir(mode=0o700)
    baseline=args.out/'unchanged-timing'
    sys.argv=[str(here/'build_q38_q4_kernel_timing.py'),str(args.sdk),str(baseline),str(args.width)]
    runpy.run_path(sys.argv[0],run_name='__main__')
    commands=[[arg.replace(str(baseline),str(args.out)) for arg in row.split('\t')]
              for row in (baseline/'compile-argv.txt').read_text().splitlines()]
    # SDK's size build retains a frame pointer even in these private,
    # freestanding routines. Remove that RV32-only overhead; no unwinder
    # exists here and the audited clock object remains byte unchanged.
    commands[0][1:1]=['-DPIONEER_Q38_Q4_PAIR_ISSUE_IMAGE=1','-fomit-frame-pointer']
    assembly=next(row for row in commands if row[0].endswith('-as'))
    assembly[1:1]=['--defsym','PIONEER_Q38_Q4_PAIR_ISSUE_IMAGE=1']
    for command in commands:
        subprocess.run(command,cwd=args.out,check=True)
    tool=str(args.sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku-')
    elf=args.out/'program.elf'
    assert not subprocess.check_output([tool+'nm','-u',str(elf)]).strip()
    symbols=subprocess.check_output([tool+'nm','-n','-S',str(elf)],text=True)
    assert not re.search(r'^[0-9a-f]+ [0-9a-f]+ [bBdDgGsS] ',symbols,re.M)
    (args.out/'program.symbols').write_text(symbols)
    (args.out/'program.disassembly').write_bytes(subprocess.check_output([tool+'objdump','-d',str(elf)]))
    (args.out/'compile-argv.txt').write_text('\n'.join('\t'.join(row) for row in commands)+'\n')
    code=(args.out/'text.bin').read_bytes()
    assert len(code)==7820 and len(code)<=8192,'original 8KiB ROM bound'
    assert hashlib.sha256(code).hexdigest()==INSPECT_PINS[args.width][0]
    assert elf.stat().st_size==20236 and hashlib.sha256(elf.read_bytes()).hexdigest()==INSPECT_PINS[args.width][1]
    assert (args.out/'clock.o').read_bytes()==(baseline/'clock.o').read_bytes()
    clock=re.search(r'^([0-9a-f]+) ([0-9a-f]+) T q38_q4_kernel_clock$',symbols,re.M)
    assert clock
    start,size=(int(value,16) for value in clock.groups())
    csrs=[]
    for offset in range(0,len(code),4):
        word=int.from_bytes(code[offset:offset+4],'little')
        if word&0x7f==0x73:
            assert start<=0x70000+offset<start+size
            assert word&0x000ff07f==0x2073 and (word>>7)&31
            csrs.append((offset,word>>20))
    assert [csr for _,csr in csrs]==[0xb80,0xb00,0xb80]
    assert csrs[1][0]==csrs[0][0]+4 and csrs[2][0]==csrs[0][0]+8
    for unit in ('program','clock'):
        frames=[line.split('\t') for line in (args.out/(unit+'.su')).read_text().splitlines()]
        assert frames and all(len(row)==3 and row[2]=='static' and int(row[1])<=4096 for row in frames)
    assert (args.out/'boot.bin').read_bytes()==(baseline/'boot.bin').read_bytes()
    for name in ('boot.bin','text.bin','program.elf'):
        data=(args.out/name).read_bytes()
        print('Q4_PAIR_ISSUE_INSPECT',args.width,name,len(data),hashlib.sha256(data).hexdigest(),flush=True)
    print('BUILD_ONLY same 8KiB/code/L1/math/NoC bounds;NO_NATIVE_OWNER_OR_HARDWARE_SPEED',flush=True)


if __name__=='__main__':main()
