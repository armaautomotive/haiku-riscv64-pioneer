#!/usr/bin/env python3
"""NEW OFFLINE persistent mixed worker; no native install or card IO."""
import hashlib
import os
import pathlib
import re
import runpy
import subprocess
import sys

if len(sys.argv) != 3 or any(k.startswith('PIONEER_') for k in os.environ):
    raise SystemExit(2)
sdk, out = map(pathlib.Path, sys.argv[1:])
assert sdk.is_absolute() and out.is_absolute() and not out.exists() and not out.is_symlink()
here = pathlib.Path(__file__).resolve().parent
out.mkdir(mode=0o700)
saved = list(sys.argv)
try:
    sys.argv = [str(here/'build_qwen38_resident_gdn_output_operator_service_image.py'), str(sdk), str(out/'unchanged-worker')]
    old = runpy.run_path(sys.argv[0], run_name='__main__')
finally:
    sys.argv = saved
# The ancestor builder authenticates its entire chain. Independently pin
# its final SIX sections before deriving flags, NEVER select its native role.
for name, size, digest in (
    ('boot', 4, 'e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b'),
    ('text', 7940, 'd726337fe94d7d94812aaf5e67f3d4e328a21873cb27aa982dc5315bb6294b07'),
    ('affine', 5360, 'fcdf8ec5b607550db93fda5a86830951e2518d5580b11f4538fb6eead0eb3b7b'),
    ('ffn', 16352, 'f17def1072f3e9ceaff2a7f1f66596ca34efc4d7ed9e3ea0d52dbc4cf9a7acb9'),
    ('service', 10300, 'd00147de57e51e096760da0e6d25476692646807bcb0dc52d5a02b1263657254'),
    ('gdn', 35396, '36458af185c3500d1cd1b1170672c2138910aff836bd8a6a407b07c68a80b93d'),
):
    data = (out/'unchanged-worker'/(name+'.bin')).read_bytes()
    assert len(data) == size and hashlib.sha256(data).hexdigest() == digest, name
base = old['base']
cross, flags = base['cross'], [a for a in base['flags'] if a != '-O2']
fixed = ['-ffixed-s11', '-DPIONEER_Q38_MIXED_CAPSULE_IMAGE=1', '-include', str(here/'PioneerQ38MixedCapsuleHook.h')]
units = [(s, n, e, o) for s, n, e, o in base['units'] if n != 'program']
units += [
    ('blackhole_qwen38_resident_operator_service.cpp', 'program', [*base['peer'], *old['roles']], '-O2'),
    ('blackhole_qwen38_resident_gdn_output_service.cpp', 'gdn_output', old['roles'], '-Os'),
    ('blackhole_qwen38_mixed_worker.cpp', 'mixed', ['-DPIONEER_Q38_MIXED_WORKER_IMAGE=1'], '-Os'),
    ('blackhole_qwen38_dense_partition.cpp', 'dense', ['-DPIONEER_Q38_MIXED_WORKER_IMAGE=1', '-DPIONEER_Q38_MIXED_DENSE_CAPSULE_IMAGE=1', '-Dscalar_fault_park=block_fault_park'], '-O2'),
]
def run(*args):
    subprocess.run([str(a) for a in args], check=True)
for source, name, extra, optimize in units:
    run(str(cross)+'-g++', *flags, optimize, *fixed, *extra, '-c', here/source, '-o', out/(name+'.o'))
run(str(cross)+'-as', '-march=rv32imf', '-mabi=ilp32', '-mno-relax', here/'blackhole_qwen38_mixed_worker_start.S', '-o', out/'start.o')
# Derive only the OFFLINE capsule placement, not a native snapshot grant.
# New code regions and boot PC need their own complete source/init/clock/
# GDDR+L1+Tensix+NoC lifetime owner before any hardware use.
link = (here/'blackhole_qwen38_resident_gdn_operator_service.ld').read_text()
assert link.count('SECTIONS {') == 1 and link.count('ASSERT(block_entry==0x72000,"fixed entry")') == 1
link = link.replace('SECTIONS {', '''SECTIONS {
 .mixed 0x20000 : { KEEP(*(.q38_mixed_start)) *mixed.o(.text .text.* .rodata .rodata.* .srodata .srodata.*) . = ALIGN(4); }
 .dense 0x40000 : { *dense.o(.text .text.* .rodata .rodata.* .srodata .srodata.*) . = ALIGN(4); }
 ASSERT(SIZEOF(.mixed)>0 && SIZEOF(.mixed)<=0x10000,"mixed front end exceeds NEW extent")
 ASSERT(SIZEOF(.dense)>0 && SIZEOF(.dense)<=0x10000,"dense capsule exceeds NEW extent")''')
link = link.replace('ASSERT(block_entry==0x72000,"fixed entry")', 'ASSERT(block_entry==0x20000,"NEW mixed entry, not old native PC")')
assert link.count('.ffn 0x64000') == 1
link = link.replace('.ffn 0x64000', '.ffn 0x60000')
link = link.replace('SIZEOF(.ffn)<=0x4000', 'SIZEOF(.ffn)<=0x6000')
(out/'program.ld').write_text(link)
run(str(cross)+'-ld', '-m', 'elf32lriscv', '--no-relax', '--gc-sections', '-T', out/'program.ld',
    out/'start.o', *[out/(n+'.o') for _, n, _, _ in units], '-Map', out/'program.map', '-o', out/'program.elf')
sections = ('boot', 'mixed', 'dense', 'text', 'affine', 'ffn', 'service', 'gdn')
for section in sections:
    run(str(cross)+'-objcopy', '-O', 'binary', '-j', '.'+section, out/'program.elf', out/(section+'.bin'))
for tool, args, name in (('nm', ['-n'], 'symbols'), ('nm', ['-u'], 'undefined'), ('objdump', ['-d'], 'disassembly'), ('objdump', ['-h'], 'sections')):
    with (out/('program.'+name)).open('w') as stream:
        subprocess.run([str(cross)+'-'+tool, *args, str(out/'program.elf')], stdout=stream, check=True)
assert not (out/'program.undefined').read_bytes()
symbols = (out/'program.symbols').read_text()
assert not re.search(r'^[0-9a-f]+ [bBdDgGsS] ', symbols, re.M)
assert 'q38_persistent_main' not in symbols and 'scalar_main' not in symbols, 'old lifecycle must be unreachable'
dis = (out/'program.disassembly').read_text()
assert not re.search(r'\b(?:divu?|remu?|mulh[su]*|fdiv\.[sd]|fsqrt\.[sd]|fmadd|fmsub|fnmadd|fnmsub|fcvt\.wu?\.s|ebreak)\b', dis)
reserved = [line for line in dis.splitlines() if re.search(r'\bs11\b', line)]
assert len(reserved) == 2 and sum(bool(re.search(r'\bmv\s+s11,\w+\s*$', line)) for line in reserved) == 1
assert sum(bool(re.search(r'\bmv\s+\w+,s11\s*$', line)) for line in reserved) == 1, 's11 must have ONE private producer and ONE seal reader'
run(sys.executable, here/'tests/pioneer_q38_gdn_rendezvous_csr_gate.py', out/'program.disassembly')
rows = (out/'program.sections').read_text().splitlines()
for i, row in enumerate(rows):
    m = re.match(r'\s*\d+\s+(\S+)\s+([0-9a-fA-F]+)\s+', row)
    if m and 'ALLOC' in rows[i+1]:
        assert m[1][1:] in sections, 'unexported allocated bytes: '+row
for _, name, _, _ in units:
    frames = [int(line.split('\t')[1]) for line in (out/(name+'.su')).read_text().splitlines()]
    assert frames and max(frames) <= (2048 if name in ('mixed', 'dense') else 4096), (name, max(frames))
for name in [*(s+'.bin' for s in sections), 'program.elf']:
    data = (out/name).read_bytes()
    print(hashlib.sha256(data).hexdigest()+' '+name+' bytes'+str(len(data)))
print('PASS NEW OFFLINE QMW1 front end/ONE global nonce/returning matrix+Conv+GDN+three-phase dense/capsules reserve s11/private stack seal/no reset between jobs;NO native install/model/speed grant')
