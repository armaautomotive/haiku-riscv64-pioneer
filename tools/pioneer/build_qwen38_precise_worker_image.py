#!/usr/bin/env python3
"""Distinct QGW2+QPA1 worker. Rebuild/pin QGW1 first; BUILD ONLY."""
import hashlib
import os
import pathlib
import re
import runpy
import subprocess
import sys

if len(sys.argv) != 3 or any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ):
    raise SystemExit(2)
sdk, out = map(pathlib.Path, sys.argv[1:])
here = pathlib.Path(__file__).resolve().parent
assert sdk.is_absolute() and out.is_absolute() and not out.exists() and not out.is_symlink()
out.mkdir(mode=0o700)
saved = sys.argv[:]
try:
    sys.argv = [str(here/'build_qwen38_global_worker_image.py'), str(sdk), str(out/'unchanged-global')]
    parent = runpy.run_path(sys.argv[0], run_name='__main__')
finally:
    sys.argv = saved
cross, flags = parent['cross'], parent['flags']


def run(*args):
    subprocess.run(list(map(str, args)), check=True)


for source, name, optimize in (('blackhole_qwen38_global_worker.cpp', 'global', '-Os'),
                                ('blackhole_qwen38_precise_persistent_activation.cpp', 'precise', '-O2')):
    run(str(cross)+'-g++', *flags, optimize, '-ffixed-s11', '-DPIONEER_Q38_PRECISE_WORKER_IMAGE=1',
        '-c', here/source, '-o', out/(name+'.o'))
run(str(cross)+'-as', '-march=rv32imf', '-mabi=ilp32', '-mno-relax',
    here/'blackhole_qwen38_precise_worker_start.S', '-o', out/'start.o')
link = (out/'unchanged-global/program.ld').read_text()
assert link.count('SECTIONS {') == 1
link = link.replace('SECTIONS {', '''SECTIONS {
 .precise 0x30000 : { *precise.o(.text .text.* .rodata .rodata.* .srodata .srodata.*) . = ALIGN(4); }
 ASSERT(SIZEOF(.precise)>0 && SIZEOF(.precise)<=0x10000,"QPA1 capsule exceeds NEW extent;NO native mapping grant")''')
(out/'program.ld').write_text(link)
run(str(cross)+'-ld', '-m', 'elf32lriscv', '--no-relax', '--gc-sections', '-T', out/'program.ld',
    out/'start.o', out/'global.o', out/'precise.o',
    *[out/'unchanged-global/qualified-math'/(name+'.o') for name in parent['units']],
    '-Map', out/'program.map', '-o', out/'program.elf')
sections = ('boot', 'globalworker', 'precise', 'dense', 'text', 'affine', 'ffn', 'service', 'gdn')
for section in sections:
    run(str(cross)+'-objcopy', '-O', 'binary', '-j', '.'+section, out/'program.elf', out/(section+'.bin'))
symbols = subprocess.check_output([str(cross)+'-nm', '-n', str(out/'program.elf')], text=True)
assert not subprocess.check_output([str(cross)+'-nm', '-u', str(out/'program.elf')], text=True).strip()
assert not re.search(r'^[0-9a-f]+ [bBdDgGsS] ', symbols, re.M)
for name in ('q38_global_worker_main', 'q38_mixed_seal', 'q38_precise_persistent_activation'):
    assert re.search(r' T '+name+'$', symbols, re.M), name
dis = subprocess.check_output([str(cross)+'-objdump', '-d', str(out/'program.elf')], text=True)
assert not re.search(r'\b(?:divu?|remu?|mulh[su]*|fdiv\.[sd]|fsqrt\.[sd]|fmadd|fmsub|fnmadd|fnmsub|fcvt\.wu?\.s|ebreak)\b', dis)
reserved = [line for line in dis.splitlines() if re.search(r'\bs11\b', line)]
assert len(reserved) == 2
assert sum(bool(re.search(r'\bmv\s+s11,\w+\s*$', line)) for line in reserved) == 1
assert sum(bool(re.search(r'\bmv\s+\w+,s11\s*$', line)) for line in reserved) == 1
for name, limit in (('global', 2048), ('precise', 2128)):
    frames = [int(line.split('\t')[1]) for line in (out/(name+'.su')).read_text().splitlines()]
    assert frames and max(frames) <= limit, (name, max(frames))
(out/'program.symbols').write_text(symbols)
(out/'program.disassembly').write_text(dis)
run(sys.executable, here/'tests/pioneer_q38_gdn_rendezvous_csr_gate.py', out/'program.disassembly')
rows = subprocess.check_output([str(cross)+'-objdump', '-h', str(out/'program.elf')], text=True).splitlines()
for i, row in enumerate(rows):
    match = re.match(r'\s*\d+\s+(\S+)\s+([0-9a-fA-F]+)\s+', row)
    if match and 'ALLOC' in rows[i+1]:
        assert match[1][1:] in sections, row
for name in (*(s+'.bin' for s in sections), 'program.elf'):
    data = (out/name).read_bytes()
    assert data and not len(data) % 4
    print(hashlib.sha256(data).hexdigest()+' '+name+' bytes'+str(len(data)))
print('BUILD ONLY DISTINCT QGW2/QPA1/returning precise256 activation+native operands/128B freshness line/'+
      'private activation-before-down ordering/unchanged QGW1 ancestor pinned;NO ROOT,all64,native,physical,Work or TPS grant')
