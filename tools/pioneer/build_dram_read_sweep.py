#!/usr/bin/env python3
"""Build an isolated device read-sweep leaf; NO driver/install/hardware route."""
import hashlib
import os
import pathlib
import re
import subprocess
import sys

assert len(sys.argv) in (3,4)
queued = len(sys.argv)==4
assert not queued or sys.argv[3]=='--queued'
assert not any(key.startswith(('PIONEER_', 'TT_PIONEER_')) for key in os.environ)
sdk, out = map(pathlib.Path, sys.argv[1:3])
assert sdk.is_absolute() and out.is_absolute() and not out.exists() and not out.is_symlink()
here = pathlib.Path(__file__).resolve().parent
tool = str(sdk / 'cross-tools-riscv64/bin/riscv64-unknown-haiku') + '-'
out.mkdir(mode=0o700)
flags = ['-std=c++11', '-O2', '-Wall', '-Wextra', '-Werror', '-ffreestanding',
         '-fno-exceptions', '-fno-rtti', '-fno-stack-protector', '-fno-pic', '-fno-pie',
         '-ffunction-sections', '-fdata-sections', '-fno-unwind-tables',
         '-fno-asynchronous-unwind-tables', '-msmall-data-limit=0', '-mno-relax',
         '-march=rv32im_zicsr', '-mabi=ilp32', '-fstack-usage']

def run(*args):
    subprocess.run(list(map(str, args)), check=True)

source='blackhole_dram_queued_read_sweep.cpp' if queued else 'blackhole_dram_read_sweep.cpp'
run(tool+'g++', *flags, '-c', here/source, '-o', out/'sweep.o')
run(tool+'as', '-march=rv32im_zicsr', '-mabi=ilp32', '-mno-relax',
    here/'blackhole_dram_read_sweep_start.S', '-o', out/'start.o')
run(tool+'ld', '-m', 'elf32lriscv', '--no-relax', '--gc-sections',
    '-T', here/'blackhole_dram_read_sweep.ld', out/'start.o', out/'sweep.o',
    '-Map', out/'program.map', '-o', out/'program.elf')
elf = out/'program.elf'
assert not subprocess.check_output([tool+'nm', '-u', str(elf)], text=True).strip()
symbols = subprocess.check_output([tool+'nm', '-n', str(elf)], text=True)
assert not re.search(r'^[0-9a-f]+ [bBdDgGsS] ', symbols, re.M)
dis = subprocess.check_output([tool+'objdump', '-d', str(elf)], text=True)
assert not re.search(r'\b(?:divu?|remu?|mulh[su]*|ebreak|ecall|csrrw|csrrc|csrrsi|csrrci|csrrwi)\b', dis)
# Exactly three read-only CSR instructions in the bounded sampler, no other CSR.
csr = [row for row in dis.splitlines() if re.search(r'\bcsrr', row)]
assert len(csr) == 3 and all(re.search(r'\bcsrr\s+\w+,mcycleh?\s*$', row) for row in csr), csr
for row in csr:
    instruction = int(row.split(':', 1)[1].split()[0], 16)
    assert instruction & 127 == 0x73 and instruction >> 12 & 7 == 2
    assert instruction >> 15 & 31 == 0 and instruction >> 20 in (0xb00, 0xb80)
frames = [int(row.split('\t')[1]) for row in (out/'sweep.su').read_text().splitlines()]
assert frames and max(frames) <= 1024
(out/'program.symbols').write_text(symbols)
(out/'program.disassembly').write_text(dis)
for section in ('boot', 'text'):
    path = out/(section+'.bin')
    run(tool+'objcopy', '-O', 'binary', '-j', '.'+section, elf, path)
    data = path.read_bytes()
    assert data and not len(data)%4
    print(section, len(data), hashlib.sha256(data).hexdigest(), flush=True)
print('BUILD_ONLY '+('queued' if queued else 'single-outstanding')+' single-worker READ sweep;NO native ownership/driver/STOP/restore/'
      'whole-memory-integrity/physical bandwidth/inference/Work grant', flush=True)
