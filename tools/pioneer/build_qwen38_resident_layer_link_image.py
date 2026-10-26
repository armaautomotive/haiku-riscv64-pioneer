#!/usr/bin/env python3
"""Separate linked layer->next-input program; NO native registration or device IO."""
import hashlib
import os
import pathlib
import re
import runpy
import struct
import subprocess
import sys

if len(sys.argv) != 3 or any(k.startswith('PIONEER_') for k in os.environ):
    raise SystemExit(2)
sdk, output = map(pathlib.Path, sys.argv[1:])
assert sdk.is_absolute() and output.is_absolute() and not output.exists() and not output.is_symlink()
here = pathlib.Path(__file__).resolve().parent
output.mkdir(mode=0o700)
saved = list(sys.argv)
try:
    sys.argv = [str(here/'build_qwen38_resident_layer_image.py'), str(sdk), str(output/'unchanged-layer')]
    base = runpy.run_path(sys.argv[0], run_name='__main__')
finally:
    sys.argv = saved
prior = output/'unchanged-layer'
pins = runpy.run_path(str(here/'tests/pioneer_q38_resident_layer_image_common.py'))['PROGRAM_PINS']
for section, size, digest in pins:
    data = (prior/(section+'.bin')).read_bytes()
    assert len(data) == size and hashlib.sha256(data).hexdigest() == digest, section
cross, flags = base['cross'], base['flags']
def run(*args):
    subprocess.run([str(a) for a in args], check=True)
run(str(cross)+'-g++', *flags, '-O2', '-DPIONEER_Q38_RESIDENT_LAYER_LINK_IMAGE=1',
    '-c', here/'blackhole_qwen38_resident_layer_link.cpp', '-o', output/'link.o')
run(str(cross)+'-as', '-march=rv32imf', '-mabi=ilp32', '-mno-relax',
    here/'blackhole_qwen38_resident_layer_link_start.S', '-o', output/'start.o')
run(str(cross)+'-ld', '-m', 'elf32lriscv', '--no-relax', '--gc-sections', '-T',
    here/'blackhole_qwen38_resident_layer_link.ld', output/'start.o', prior/'root.o',
    *[prior/(n+'.o') for n in ('matrix', 'norm', 'ffn', 'gather')],
    prior/'unchanged-output/unchanged-cohort/root_conv.o', output/'link.o', '-Map', output/'program.map', '-o', output/'program.elf')
sections = ('boot', 'text', 'conv', 'matrix', 'cohort', 'norm', 'link', 'ffn')
for section in sections:
    run(str(cross)+'-objcopy', '-O', 'binary', '-j', '.'+section, output/'program.elf', output/(section+'.bin'))
for tool, args, name in (('nm', ['-n'], 'program.symbols'), ('nm', ['-u'], 'program.undefined'),
                         ('objdump', ['-d'], 'program.disassembly'), ('objdump', ['-h'], 'program.sections')):
    with (output/name).open('w') as stream:
        subprocess.run([str(cross)+'-'+tool, *args, str(output/'program.elf')], stdout=stream, check=True)
assert not (output/'program.undefined').read_bytes()
symbols = (output/'program.symbols').read_text()
disassembly = (output/'program.disassembly').read_text()
for name, address in (('cohort_entry', 0x72000), ('cohort_done', 0x7200c), ('cohort_park', 0x72038), ('cohort_fault_park', 0x72060)):
    assert re.search('^'+format(address, '08x')+' T '+name+'$', symbols, re.M), name
for name in ('q38_resident_cohort_main', 'q38_resident_layer_link_main', 'q38_resident_layer_link_capture', 'q38_resident_layer_link_once'):
    assert re.search(r'^[0-9a-f]+ T '+name+'$', symbols, re.M), name
assert not re.search(r'^[0-9a-f]+ [bBdDgGsS] ', symbols, re.M)
assert not re.search(r'\b(?:divu?|remu?|mulh[su]*|fdiv\.[sd]|fsqrt\.[sd]|fmadd|fmsub|fnmadd|fnmsub|fcvt\.wu?\.s)\b', disassembly)
assert (output/'boot.bin').read_bytes() == (prior/'boot.bin').read_bytes()
terminal = (output/'text.bin').read_bytes()
assert len(terminal) == 128 and terminal[0x38:0x40] == struct.pack('<II', 0x0000006f, 0xffdff06f)
assert terminal[0x40:0x60] == bytes(32)
assert terminal != (prior/'text.bin').read_bytes()  # NEW QLK1/QLKF, not a native69 terminal
for section in ('conv', 'matrix', 'cohort', 'norm', 'ffn'):
    assert (output/(section+'.bin')).read_bytes() == (prior/(section+'.bin')).read_bytes(), section
csr = []
for line in disassembly.splitlines():
    match = re.match(r'\s*([0-9a-f]+):\s+([0-9a-f]{8})\s', line)
    if match and int(match[2], 16)&127 == 0x73:
        address, word = int(match[1], 16), int(match[2], 16)
        assert (word>>12)&7 == 2 and (word>>15)&31 == 0 and (word>>7)&31 and word>>20 in (0xb00, 0xb80)
        csr.append((address, word>>20))
assert len(csr) == 6 and [v for _, v in csr] == [0xb80, 0xb00, 0xb80]*2
for group, (first, last) in enumerate(((0x68000, 0x6e000), (0x15c000, 0x17e000))):
    a = csr[group*3:group*3+3]
    assert first <= a[0][0] and a[2][0] < last and a[1][0] == a[0][0]+4 and a[2][0] == a[0][0]+8
rows = (output/'program.sections').read_text().splitlines()
for i, row in enumerate(rows):
    match = re.match(r'\s*\d+\s+(\S+)\s+([0-9a-fA-F]+)\s+', row)
    if match and i+1 < len(rows) and 'ALLOC' in rows[i+1]:
        assert match[1][1:] in sections, row
maximum = 0
for obj in ('link',):
    frames = base['frames'](output/(obj+'.su'))
    assert frames
    for function, size in frames.items():
        assert size <= 2048, (obj, function, size)
        maximum = max(maximum, size)
for section in sections:
    data = (output/(section+'.bin')).read_bytes()
    print(hashlib.sha256(data).hexdigest()+' '+section+' bytes'+str(len(data)))
# Keep the standalone builder independent of Python's caller import path.
sys.path.insert(0, str(here/'tests'))
try:
    from pioneer_q38_resident_layer_link_image_common import verify
    verify(output)
finally:
    sys.path.pop(0)
print('PASS NEW COMPILED device layer->next learned-RMS+raw-residual handoff;maxNewFrame'+str(maximum))
print('Old QLR1/peers/norm/FFN bytes preserved;NO host callback or intermediate readback;new native ownership/initialization/qualification OWED;NOT full64layer/attention/token-loop/Work/speed')
