#!/usr/bin/env python3
"""OFFLINE complete ROOT0+119 Conv->96 cooperative GDN schedule,NO card."""
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
saved = list(sys.argv)
try:
    sys.argv = [str(here/'build_qwen38_resident_gdn_operator_service_image.py'), str(sdk), str(output/'exact-companion')]
    worker = runpy.run_path(sys.argv[0], run_name='__main__')
finally:
    sys.argv = saved
for section, digest in (
    ('boot', 'e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b'),
    ('text', '47ff39e8f9709f9e87469e44919b374ccfbea3edd3f1f1f1b3020ecfd545c30b'),
    ('affine', 'd8734c6a6e1cea65e8abac370292033f8be1406fa063c0f84501646aaaedca5c'),
    ('ffn', 'f17def1072f3e9ceaff2a7f1f66596ca34efc4d7ed9e3ea0d52dbc4cf9a7acb9'),
    ('service', '3498f0507019f44b4c76101ca8df2df84dc4d57e3a288703c437fb441fa0eb43'),
    ('gdn', '67f202ea9afbf902488af532418c876c8983e146b21d272b1cbf88108b412648'),
):
    assert hashlib.sha256((output/'exact-companion'/(section+'.bin')).read_bytes()).hexdigest() == digest
symbols = (output/'exact-companion/program.symbols').read_text()
for symbol, address in (('block_peer_wait_load', 0x7204c), ('block_peer_wait_branch', 0x72050),
                        ('block_peer_stopped', 0x72060), ('block_fault_park', 0x72080)):
    assert re.search('^'+format(address, '08x')+' T '+symbol+'$', symbols, re.M)
cross = worker['cross']
flags = [a for a in worker['flags'] if a != '-O2']
roles = [*worker['base']['roles'], '-DPIONEER_Q38_RESIDENT_COHORT_ROOT_IMAGE=1',
         '-include', str(here/'PioneerQ38ResidentCohortImageRole.h')]
for source, name, extra in (
    ('blackhole_qwen38_resident_cohort.cpp', 'root', []),
    ('blackhole_qwen38_conv_batch_silu_precision.cpp', 'root_conv',
     ['-Dscalar_main=q38_cohort_conv_once', '-Dscalar_fault_park=cohort_fault_park']),
):
    subprocess.run([str(cross)+'-g++', *flags, '-Os', *roles, *extra,
                    '-c', str(here/source), '-o', str(output/(name+'.o'))], check=True, env=clean)
subprocess.run([str(cross)+'-as', '-march=rv32imf', '-mabi=ilp32', '-mno-relax',
                str(here/'blackhole_qwen38_resident_cohort_start.S'), '-o', str(output/'start.o')], check=True, env=clean)
subprocess.run([str(cross)+'-ld', '-m', 'elf32lriscv', '--no-relax', '--gc-sections', '-T',
                str(here/'blackhole_qwen38_resident_cohort.ld'), *[str(output/(n+'.o')) for n in ('start', 'root', 'root_conv')],
                '-Map', str(output/'program.map'), '-o', str(output/'program.elf')], check=True, env=clean)
for section in ('boot', 'text', 'conv', 'cohort'):
    subprocess.run([str(cross)+'-objcopy', '-O', 'binary', '-j', '.'+section,
                    str(output/'program.elf'), str(output/(section+'.bin'))], check=True, env=clean)
for tool, argv, name in (('nm', ['-n'], 'program.symbols'), ('nm', ['-u'], 'program.undefined'),
                         ('objdump', ['-d'], 'program.disassembly')):
    with (output/name).open('w') as stream:
        subprocess.run([str(cross)+'-'+tool, *argv, str(output/'program.elf')], stdout=stream, check=True, env=clean)
assert not (output/'program.undefined').read_bytes()
section_table = subprocess.check_output([str(cross)+'-objdump', '-h', str(output/'program.elf')], text=True, env=clean)
rows = section_table.splitlines()
for i, row in enumerate(rows):
    match = re.match(r'\s*\d+\s+(\S+)\s+([0-9a-fA-F]+)\s+', row)
    if match and i+1 < len(rows) and 'ALLOC' in rows[i+1]:
        assert match[1] in ('.boot', '.text', '.conv', '.cohort'), 'unexported allocated section: '+row
assert not re.search(r'^[0-9a-f]+ [bBdDgGsS] ', (output/'program.symbols').read_text(), re.M)
assert not re.search(r'\b(?:divu?|remu?|mulh[su]*|fdiv\.[sd]|fsqrt\.[sd]|fmadd|fmsub|fnmadd|fnmsub|fcvt\.wu?\.s|csr\w*)\b', (output/'program.disassembly').read_text())
frames = worker['base']['frames']
maximum = max(size for name in ('root', 'root_conv') for size in frames(output/(name+'.su')).values())
assert maximum <= 2048
assert (output/'boot.bin').read_bytes() == struct.pack('<I', 0x0006c06f)
for section in ('boot', 'text', 'conv', 'cohort'):
    data = (output/(section+'.bin')).read_bytes()
    print(hashlib.sha256(data).hexdigest()+' '+section+' bytes'+str(len(data)))
print('PASS OFFLINE cohort ROOT+SIX actual root Conv groups compile/maxFrame'+str(maximum)+'/ALL6 companion sections EXACT FIRST')
print('NEW c8000..14b600 manifest/shadow/ROM footprint;NO existing native snapshot/owner/card/all64layers/token-loop or speed grant')
