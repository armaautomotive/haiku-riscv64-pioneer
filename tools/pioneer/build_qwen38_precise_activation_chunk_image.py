#!/usr/bin/env python3
"""Isolated QAC1 precise activation+packing, no native/Work/default change."""
import hashlib
import os
import pathlib
import re
import subprocess
import sys

if len(sys.argv) != 3 or any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ):
    raise SystemExit(2)
sdk, out = map(pathlib.Path, sys.argv[1:])
here = pathlib.Path(__file__).resolve().parent
assert sdk.is_absolute() and out.is_absolute() and not out.exists() and not out.is_symlink()
cross = str(sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku')
assert pathlib.Path(cross+'-g++').is_file()
out.mkdir(mode=0o700)


def run(*args):
    subprocess.run(list(map(str, args)), check=True)


run(cross+'-g++', '-std=c++11', '-O2', '-Wall', '-Wextra', '-Werror', '-ffreestanding',
    '-fno-exceptions', '-fno-rtti', '-fno-stack-protector', '-fno-pic', '-fno-pie',
    '-ffp-contract=off', '-ffunction-sections', '-fdata-sections', '-fno-unwind-tables',
    '-fno-asynchronous-unwind-tables', '-msmall-data-limit=0', '-mno-relax', '-march=rv32imf',
    '-mabi=ilp32', '-fstack-usage', '-c', here/'blackhole_qwen38_precise_activation_chunk.cpp',
    '-o', out/'program.o')
run(cross+'-as', '-march=rv32imf', '-mabi=ilp32', '-mno-relax',
    here/'blackhole_qwen38_precise_activation_chunk_start.S', '-o', out/'start.o')
run(cross+'-ld', '-m', 'elf32lriscv', '--no-relax', '--gc-sections', '-T', here/'blackhole_scalar.ld',
    out/'start.o', out/'program.o', '-Map', out/'program.map', '-o', out/'program.elf')
for section, name in (('text', 'code'), ('boot', 'boot')):
    run(cross+'-objcopy', '-O', 'binary', '-j', '.'+section, out/'program.elf', out/(name+'.bin'))
for tool, args, name in (('nm', ['-n'], 'symbols'), ('nm', ['-u'], 'undefined'),
                         ('objdump', ['-d'], 'disassembly'), ('objdump', ['-h'], 'sections')):
    with (out/('program.'+name)).open('w') as target:
        subprocess.run([cross+'-'+tool, *args, str(out/'program.elf')], stdout=target, check=True)
assert not (out/'program.undefined').read_bytes()
dis = (out/'program.disassembly').read_text()
symbols = (out/'program.symbols').read_text()
assert not re.search(r'^[0-9a-f]+ [bBdDgGsS] ', symbols, re.M)
assert not re.search(r'\b(?:divu?|remu?|mulh[su]*|fdiv\.[sd]|fsqrt\.[sd]|fmadd|fmsub|fnmadd|fnmsub|fcvt\.wu?\.s|ebreak)\b', dis)
for line in dis.splitlines():
    match = re.match(r'\s*[0-9a-f]+:\s+([0-9a-f]{8})\s', line)
    if match:
        assert int(match[1], 16) & 127 != 0x73
for name, address in (('scalar_entry', 0x64000), ('scalar_park', 0x64038), ('scalar_fault_park', 0x6403c)):
    assert re.search('^'+format(address, '08x')+' T '+name+'$', symbols, re.M), name
frames = [int(s.split('\t')[1]) for s in (out/'program.su').read_text().splitlines()]
# The unchanged generic projection quantizer has a 2128-byte frame here.
# This is an isolated 8192-byte local stack, NOT a kernel-stack allowance.
# Actual emitted execution below must additionally bound combined usage.
assert frames and max(frames) <= 2128
for name in ('boot.bin', 'code.bin', 'program.elf'):
    data = (out/name).read_bytes()
    assert data and not len(data) % 4
    print(hashlib.sha256(data).hexdigest()+' '+name+' bytes'+str(len(data)))
print('PASS isolated QAC1 emitted/maxFrame'+str(max(frames))+
      '/NO unsupported arithmetic,CSR,writable globals or undefined symbols;NOT executed/native/Work/TPS')
