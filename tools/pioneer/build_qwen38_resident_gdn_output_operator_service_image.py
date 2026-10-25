#!/usr/bin/env python3
"""OFFLINE all-layer returning GDN-output peer; no native/device entry."""
import hashlib
import os
import pathlib
import re
import runpy
import struct
import subprocess
import sys

if len(sys.argv) != 3 or any(k.startswith('PIONEER_') and v != '0' for k, v in os.environ.items()):
    raise SystemExit(2)
sdk, output = map(pathlib.Path, sys.argv[1:])
assert sdk.is_absolute() and output.is_absolute() and not output.exists() and not output.is_symlink()
here = pathlib.Path(__file__).resolve().parent
clean = {k: v for k, v in os.environ.items() if not k.startswith('PIONEER_')}
output.mkdir(mode=0o700)
old = output/'unchanged-worker'
saved = list(sys.argv)
try:
    sys.argv = [str(here/'build_qwen38_resident_gdn_operator_service_image.py'), str(sdk), str(old)]
    base = runpy.run_path(sys.argv[0], run_name='__main__')
finally:
    sys.argv = saved
for section, size, digest in (
    ('boot', 4, 'e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b'),
    ('text', 7912, '47ff39e8f9709f9e87469e44919b374ccfbea3edd3f1f1f1b3020ecfd545c30b'),
    ('affine', 5360, 'd8734c6a6e1cea65e8abac370292033f8be1406fa063c0f84501646aaaedca5c'),
    ('ffn', 16352, 'f17def1072f3e9ceaff2a7f1f66596ca34efc4d7ed9e3ea0d52dbc4cf9a7acb9'),
    ('service', 10300, '3498f0507019f44b4c76101ca8df2df84dc4d57e3a288703c437fb441fa0eb43'),
    ('gdn', 29860, '67f202ea9afbf902488af532418c876c8983e146b21d272b1cbf88108b412648'),
):
    data = (old/(section+'.bin')).read_bytes()
    assert len(data) == size and hashlib.sha256(data).hexdigest() == digest
cross, flags = base['cross'], list(base['flags'])
roles = [*base['roles'], '-DPIONEER_Q38_RESIDENT_GDN_OUTPUT_OPERATOR_IMAGE=1',
         '-include', str(here/'PioneerQ38ResidentGdnOutputImageRole.h')]
for source, name, extra, optimize in (
    ('blackhole_qwen38_resident_operator_service.cpp', 'program', base['peer'], '-O2'),
    ('blackhole_qwen38_resident_gdn_output_service.cpp', 'gdn_output', [], '-Os'),
):
    subprocess.run([str(cross)+'-g++', *[a for a in flags if a != '-O2'], optimize, *roles, *extra,
                    '-c', str(here/source), '-o', str(output/(name+'.o'))], check=True, env=clean)
objects = [old/'start.o', output/'program.o', output/'gdn_output.o']
objects += [old/(name+'.o') for _, name, _, _ in base['units'] if name != 'program']
subprocess.run([str(cross)+'-ld', '-m', 'elf32lriscv', '--no-relax', '--gc-sections', '-T',
                str(here/'blackhole_qwen38_resident_gdn_operator_service.ld'), *map(str, objects),
                '-Map', str(output/'program.map'), '-o', str(output/'program.elf')], check=True, env=clean)
sections = ('boot', 'text', 'affine', 'ffn', 'service', 'gdn')
for section in sections:
    subprocess.run([str(cross)+'-objcopy', '-O', 'binary', '-j', '.'+section,
                    str(output/'program.elf'), str(output/(section+'.bin'))], check=True, env=clean)
for tool, argv, name in (('nm', ['-n'], 'program.symbols'), ('nm', ['-u'], 'program.undefined'),
                         ('objdump', ['-d'], 'program.disassembly'), ('objdump', ['-h'], 'program.sections')):
    with (output/name).open('w') as stream:
        subprocess.run([str(cross)+'-'+tool, *argv, str(output/'program.elf')], stdout=stream, check=True, env=clean)
assert not (output/'program.undefined').read_bytes()
rows = (output/'program.sections').read_text().splitlines()
for i, row in enumerate(rows):
    match = re.match(r'\s*\d+\s+(\S+)\s+([0-9a-fA-F]+)\s+', row)
    if match and i+1 < len(rows) and 'ALLOC' in rows[i+1]:
        assert match[1][1:] in sections, 'unexported allocated section: '+row
assert not re.search(r'^[0-9a-f]+ [bBdDgGsS] ', (output/'program.symbols').read_text(), re.M)
assert not re.search(r'\b(?:divu?|remu?|mulh[su]*|fdiv\.[sd]|fsqrt\.[sd]|fmadd|fmsub|fnmadd|fnmsub|fcvt\.wu?\.s)\b', (output/'program.disassembly').read_text())
subprocess.run([sys.executable, str(here/'tests/pioneer_q38_gdn_rendezvous_csr_gate.py'), str(output/'program.disassembly')], check=True, env=clean)
frames = base['base']['frames']
previous = frames(old/'program.su')
maximum = 0
for name in ('program', 'gdn_output'):
    for function, size in frames(output/(name+'.su')).items():
        assert size <= 2048 or function in previous and size <= previous[function], (function, size, previous.get(function))
        if function not in previous:
            maximum = max(maximum, size)
assert (output/'boot.bin').read_bytes() == struct.pack('<I', 0x0006c06f)
for section in sections:
    data = (output/(section+'.bin')).read_bytes()
    print(hashlib.sha256(data).hexdigest()+' '+section+' bytes'+str(len(data)))
print('PASS OFFLINE all-layer GDN-output/matrix/Conv/GDN IMAGE compile/maxNewFrame'+str(maximum)+'/old SIX sections EXACT FIRST/ALL allocated bytes exported')
print('NEW immutable pins + joint native footprint/ROOT consumer still required;NO device/fullmodel/current-slot commit or speed grant')
