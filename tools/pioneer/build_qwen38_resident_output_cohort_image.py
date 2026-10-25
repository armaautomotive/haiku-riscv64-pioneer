#!/usr/bin/env python3
"""OFFLINE ROOT0+119 Conv/GDN/QOL1/full output matrix, NO card entry."""
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
    sys.argv = [str(here/'build_qwen38_resident_cohort_image.py'), str(sdk), str(output/'unchanged-cohort')]
    base = runpy.run_path(sys.argv[0], run_name='__main__')
    sys.argv = [str(here/'build_qwen38_resident_gdn_output_operator_service_image.py'), str(sdk), str(output/'exact-companion')]
    companion = runpy.run_path(sys.argv[0], run_name='__main__')
finally:
    sys.argv = saved
for section, digest in (
    ('boot', 'e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b'),
    ('text', 'ce66ec25896f1e06362bae978499b33222c7c51d603ce36e83bf0e077b804c22'),
    ('conv', 'feb352acfc6490b727e32ba236d8bfa2e9f7b8811b31ae4ee37f5e4d53624112'),
    ('cohort', 'dc7c83eafe4f64ad03137ce6889abc9a5d27e5d3f25865520d960098f3d06b07'),
):
    assert hashlib.sha256((output/'unchanged-cohort'/(section+'.bin')).read_bytes()).hexdigest() == digest
for section, digest in (
    ('boot', 'e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b'),
    ('text', 'd726337fe94d7d94812aaf5e67f3d4e328a21873cb27aa982dc5315bb6294b07'),
    ('affine', 'fcdf8ec5b607550db93fda5a86830951e2518d5580b11f4538fb6eead0eb3b7b'),
    ('ffn', 'f17def1072f3e9ceaff2a7f1f66596ca34efc4d7ed9e3ea0d52dbc4cf9a7acb9'),
    ('service', 'd00147de57e51e096760da0e6d25476692646807bcb0dc52d5a02b1263657254'),
    ('gdn', '36458af185c3500d1cd1b1170672c2138910aff836bd8a6a407b07c68a80b93d'),
):
    assert hashlib.sha256((output/'exact-companion'/(section+'.bin')).read_bytes()).hexdigest() == digest
old_matrix = output/'unchanged-matrix'
subprocess.run(['sh', str(here/'build_qwen38_projection_matrix_image.sh'), str(sdk), str(old_matrix)], check=True, env=clean)
for section, digest in (
    ('boot', 'e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b'),
    ('text', 'a28ec7d505a837c260939a9cbbbba9aa98ac4f13069045de8b73f84878221bad'),
    ('prefix', '5236efe8942629ebf00e8e7bea83b332473a3c180a09990e71b73217fd631017'),
):
    assert hashlib.sha256((old_matrix/(section+'.bin')).read_bytes()).hexdigest() == digest
cross, flags = base['cross'], list(base['flags'])
roles = [*base['roles'], '-DPIONEER_Q38_RESIDENT_OUTPUT_ROOT_IMAGE=1', '-include', str(here/'PioneerQ38ResidentOutputRootImageRole.h')]
subprocess.run([str(cross)+'-g++', *flags, '-O3', *roles, '-DPIONEER_Q38_COHORT_POLL_BACKOFF=1',
                '-c', str(here/'blackhole_qwen38_resident_cohort.cpp'), '-o', str(output/'root.o')], check=True, env=clean)
matrix_flags = ['-DPIONEER_Q38_PROJECTION_MATRIX_ROOT=1', '-DPIONEER_Q38_FFN_RESIDENT_MATRIX',
                '-DPIONEER_Q38_COMPACT_ACTIVATIONS', '-DPIONEER_Q38_REUSE_ACTIVATIONS', '-DPIONEER_Q38_WORK_COHORT_CACHE',
                '-DPIONEER_Q38_WORK_SKIP_DOT_CHECK', '-DPIONEER_Q38_WORK_SPARSE_EXPORT', '-DPIONEER_Q38_WORK_COMPACT_OUTPUT',
                '-DPIONEER_Q38_ROOT_CYCLE_PROFILE', '-DPIONEER_Q38_WORK_SENTINEL_POLL',
                '-Dblock_main=q38_resident_output_matrix_once', '-Dblock_fault_park=cohort_fault_park']
subprocess.run([str(cross)+'-g++', *flags, '-O2', '-fomit-frame-pointer', *roles, *matrix_flags,
                '-c', str(here/'blackhole_qwen38_dispatch.cpp'), '-o', str(output/'matrix.o')], check=True, env=clean)
subprocess.run([str(cross)+'-as', '-march=rv32imf', '-mabi=ilp32', '-mno-relax',
                str(here/'blackhole_qwen38_resident_output_cohort_start.S'), '-o', str(output/'start.o')], check=True, env=clean)
subprocess.run([str(cross)+'-ld', '-m', 'elf32lriscv', '--no-relax', '--gc-sections', '-T',
                str(here/'blackhole_qwen38_resident_output_cohort.ld'), str(output/'start.o'), str(output/'root.o'),
                str(output/'matrix.o'), str(output/'unchanged-cohort/root_conv.o'),
                '-Map', str(output/'program.map'), '-o', str(output/'program.elf')], check=True, env=clean)
sections = ('boot', 'text', 'conv', 'matrix', 'cohort')
for section in sections:
    subprocess.run([str(cross)+'-objcopy', '-O', 'binary', '-j', '.'+section,
                    str(output/'program.elf'), str(output/(section+'.bin'))], check=True, env=clean)
for tool, argv, name in (('nm', ['-n'], 'program.symbols'), ('nm', ['-u'], 'program.undefined'),
                         ('objdump', ['-d'], 'program.disassembly'), ('objdump', ['-h'], 'program.sections')):
    with (output/name).open('w') as stream:
        subprocess.run([str(cross)+'-'+tool, *argv, str(output/'program.elf')], stdout=stream, check=True, env=clean)
assert not (output/'program.undefined').read_bytes()
symbols = (output/'program.symbols').read_text()
for symbol,address in (('cohort_entry',0x72000),('cohort_done',0x7200c),('cohort_park',0x72038),
                       ('cohort_park_frontend',0x7203c),('cohort_fault_park',0x72060)):
    assert re.search('^'+format(address,'08x')+' T '+symbol+'$',symbols,re.M),(symbol,address)
terminal = (output/'text.bin').read_bytes()
assert len(terminal)==128 and terminal[0x38:0x40]==struct.pack('<II',0x0000006f,0xffdff06f)
assert terminal[0x40:0x60]==bytes(32) # disjoint non-terminal padding,NEVER accepted PC
assert terminal[0x60:0x64]==struct.pack('<I',0x001802b7) and terminal[-4:]==struct.pack('<I',0x0000006f)
assert not re.search(r'^[0-9a-f]+ [bBdDgGsS] ', (output/'program.symbols').read_text(), re.M)
assert not re.search(r'\b(?:divu?|remu?|mulh[su]*|fdiv\.[sd]|fsqrt\.[sd]|fmadd|fmsub|fnmadd|fnmsub|fcvt\.wu?\.s)\b', (output/'program.disassembly').read_text())
subprocess.run([sys.executable, str(here/'tests/pioneer_q38_gdn_rendezvous_csr_gate.py'), str(output/'program.disassembly')], check=True, env=clean)
rows = (output/'program.sections').read_text().splitlines()
for i, row in enumerate(rows):
    match = re.match(r'\s*\d+\s+(\S+)\s+([0-9a-fA-F]+)\s+', row)
    if match and i+1 < len(rows) and 'ALLOC' in rows[i+1]:
        assert match[1][1:] in sections, 'unexported allocation: '+row
frames = base['frames']
maximum = max(frames(output/'root.su').values())
assert maximum <= 2048, maximum
old_frames = frames(old_matrix/'program.su')
old_main = [size for name, size in old_frames.items() if name.endswith(':unsigned int block_main()')]
assert len(old_main) == 1
matrix_max = 0
for function, size in frames(output/'matrix.su').items():
    if function.endswith(':unsigned int q38_resident_output_matrix_once()'):
        assert size <= old_main[0], (function, size, old_main[0])
    else:
        assert size <= 2048, (function, size)
    matrix_max = max(matrix_max, size)
assert (output/'boot.bin').read_bytes() == struct.pack('<I', 0x0006c06f)
for section in sections:
    data = (output/(section+'.bin')).read_bytes()
    print(hashlib.sha256(data).hexdigest()+' '+section+' bytes'+str(len(data)))
print('PASS OFFLINE full Conv/GDN/QOL1/K6144->5120 ROOT compile/maxNewFrame'+str(maximum)+'/inheritedMatrixFrame'+str(matrix_max)+'/ALL old ROOT+QPM1+new companion pins EXACT FIRST')
print('NEW ROOT ROM/runtime/post-manifest snapshot required;NO native/card/initialized owner/token loop/current slot or speed grant')
