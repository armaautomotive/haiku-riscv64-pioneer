#!/usr/bin/env python3
"""OFFLINE compiler-only ROOT optimization;ALL existing audits retained."""
import hashlib
import os
import pathlib
import re
import runpy
import struct
import subprocess
import sys

if len(sys.argv) not in (3, 4, 5) or len(sys.argv) >= 4 and sys.argv[3] not in ('--O2', '--O3'):
    raise SystemExit(2)
if len(sys.argv) == 5 and sys.argv[4] != '--bounded-backoff':
    raise SystemExit(2)
if any(k.startswith('PIONEER_') and v != '0' for k, v in os.environ.items()):
    raise SystemExit(2)
sdk, output = map(pathlib.Path, sys.argv[1:3])
optimize = '-O2' if len(sys.argv) == 3 else '-'+sys.argv[3][2:]
backoff = ['-DPIONEER_Q38_COHORT_POLL_BACKOFF=1'] if len(sys.argv) == 5 else []
assert sdk.is_absolute() and output.is_absolute() and not output.exists() and not output.is_symlink()
here = pathlib.Path(__file__).resolve().parent
clean = {k: v for k, v in os.environ.items() if not k.startswith('PIONEER_')}
output.mkdir(mode=0o700)
old = output/'unchanged-full-audit'
saved = list(sys.argv)
try:
    sys.argv = [str(here/'build_qwen38_resident_cohort_image.py'), str(sdk), str(old)]
    base = runpy.run_path(sys.argv[0], run_name='__main__')
finally:
    sys.argv = saved
for section, digest in (
    ('boot', 'e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b'),
    ('text', 'ce66ec25896f1e06362bae978499b33222c7c51d603ce36e83bf0e077b804c22'),
    ('conv', 'feb352acfc6490b727e32ba236d8bfa2e9f7b8811b31ae4ee37f5e4d53624112'),
    ('cohort', 'dc7c83eafe4f64ad03137ce6889abc9a5d27e5d3f25865520d960098f3d06b07'),
):
    assert hashlib.sha256((old/(section+'.bin')).read_bytes()).hexdigest() == digest
# Compile the IDENTICAL coordinator source/role,changing ONLY optimization.
# Preserve the exact original -Os ROOT0 numerical object and startup object.
# NO runtime fast switch or audit removal;NO new native owner/selector.
cross = base['cross']
subprocess.run([str(cross)+'-g++', *base['flags'], optimize, *base['roles'], *backoff, '-c',
                str(here/'blackhole_qwen38_resident_cohort.cpp'), '-o', str(output/'root.o')], check=True, env=clean)
subprocess.run([str(cross)+'-ld', '-m', 'elf32lriscv', '--no-relax', '--gc-sections', '-T',
                str(here/'blackhole_qwen38_resident_cohort.ld'), str(old/'start.o'), str(output/'root.o'),
                str(old/'root_conv.o'), '-Map', str(output/'program.map'), '-o', str(output/'program.elf')], check=True, env=clean)
for section in ('boot', 'text', 'conv', 'cohort'):
    subprocess.run([str(cross)+'-objcopy', '-O', 'binary', '-j', '.'+section,
                    str(output/'program.elf'), str(output/(section+'.bin'))], check=True, env=clean)
for tool, argv, name in (('nm', ['-n'], 'program.symbols'), ('nm', ['-u'], 'program.undefined'),
                         ('objdump', ['-d'], 'program.disassembly'), ('objdump', ['-h'], 'program.sections')):
    with (output/name).open('w') as stream:
        subprocess.run([str(cross)+'-'+tool, *argv, str(output/'program.elf')], stdout=stream, check=True, env=clean)
assert not (output/'program.undefined').read_bytes()
assert not re.search(r'^[0-9a-f]+ [bBdDgGsS] ', (output/'program.symbols').read_text(), re.M)
assert not re.search(r'\b(?:divu?|remu?|mulh[su]*|fdiv\.[sd]|fsqrt\.[sd]|fmadd|fmsub|fnmadd|fnmsub|fcvt\.wu?\.s|csr\w*)\b', (output/'program.disassembly').read_text())
rows = (output/'program.sections').read_text().splitlines()
for i, row in enumerate(rows):
    match = re.match(r'\s*\d+\s+(\S+)\s+([0-9a-fA-F]+)\s+', row)
    if match and i+1 < len(rows) and 'ALLOC' in rows[i+1]:
        assert match[1] in ('.boot', '.text', '.conv', '.cohort'), 'unexported ALLOC section: '+row
maximum = max(base['frames'](output/'root.su').values())
assert maximum <= 2048
assert (output/'boot.bin').read_bytes() == struct.pack('<I', 0x0006c06f)
frontend = (output/'text.bin').read_bytes()
previous = (old/'text.bin').read_bytes()
assert len(frontend) == len(previous) == 92
# Only the call relocation can change when q38_resident_cohort_main moves.
# Resolve BOTH instructions to the exact emitted symbol;do not ignore them.
assert frontend[:4] == previous[:4] and frontend[12:] == previous[12:]
auipc, jalr = struct.unpack('<II', frontend[4:12])
assert auipc & 0xfff == 0x097 and jalr & 0xfffff == 0x080e7
immediate = jalr >> 20
if immediate & 0x800:
    immediate -= 0x1000
target = (0x72004+(auipc & 0xfffff000)+immediate) & 0xffffffff
symbols = (output/'program.symbols').read_text()
assert re.search('^'+format(target, '08x')+' T q38_resident_cohort_main$', symbols, re.M)
for name, address in (('cohort_entry', 0x72000), ('cohort_done', 0x7200c),
                      ('cohort_park', 0x72038), ('cohort_fault_park', 0x7203c)):
    assert re.search('^'+format(address, '08x')+' T '+name+'$', symbols, re.M)
for section in ('boot', 'text', 'conv', 'cohort'):
    data = (output/(section+'.bin')).read_bytes()
    print(hashlib.sha256(data).hexdigest()+' '+section+' bytes'+str(len(data)))
print('PASS OFFLINE '+optimize+('/bounded-backoff' if backoff else '')+' ROOT compile/maxRootFrame'+str(maximum)+'/ALL default ROOT+companion controls EXACT FIRST;ALL audits retained')
print('Numerical object is the SAME -Os object;its final linked route-table address may change. NO native/install/Work/token-loop or card-speed grant')
