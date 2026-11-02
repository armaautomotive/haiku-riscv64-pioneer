#!/usr/bin/env python3
"""Separate full returning manual-completion shader BUILD;NEVER install/run."""
import hashlib
import os
import pathlib
import re
import runpy
import struct
import subprocess
import sys

PINS = (
    ('boot.bin', 4, 'a12af9d63593a1f882750a147dd7a511ac476c5c7a55526bd4d51c685173991c'),
    ('globalworker.bin', 9232, '6d565f73e3f9856a252632ef278165fb78b477102c03a72f44f07320474e23bd'),
    ('precise.bin', 8392, '32c078029e1c74ec4bd9744ea494433f5003f63a936df05445d3066c722a7ecc'),
    ('dense.bin', 10332, 'd1749707ffe803e28dced9c7d065e763f125009678ace33b4d468109984869ad'),
    ('fetch.bin', 1292, '296274204d8ff073901f2690e4ef725b9340f80cfda09799fc0763d4c2b43b9a'),
    ('text.bin', 6260, '7eec13c93d9e64c5807d55f6ca9736ad4a467a95401c108f484eaa2299782fc8'),
    ('affine.bin', 5360, '343726cf6610d25d1f2d1c2e472e780c41f24b290ab58a67d26a6bf75ef71e57'),
    ('ffn.bin', 16456, '23ce6cc2dc07aa445b7babcac12a068dc3542001329cd24831e8e6749ff3a16f'),
    ('service.bin', 9888, '7d5a42a087bc6bf9d24319583f3655ae344828a51b1092154f7b0567b0fdb187'),
    ('gdn.bin', 35944, 'c2824d4822c69867879b09fac67f9f9849cf5f7202c1beca668584976eaf4e9c'),
    ('program.elf', 144488, '4526cb710c62e87ad513fc4aefdb11e7404fc84c5c6d6ea39caf137bfc0a3713'),
)  # Independent main/clean BUILD identity only,NOT execution authority.
args = sys.argv[1:]
inspect = '--inspect-candidate' in args
if inspect:
    assert args.count('--inspect-candidate') == 1
    args.remove('--inspect-candidate')
assert len(args) == 2 and (inspect or PINS)
assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
sdk, out = map(pathlib.Path, args)
here = pathlib.Path(__file__).resolve().parent
assert sdk.is_absolute() and out.is_absolute() and not out.exists() and not out.is_symlink()
out.mkdir(mode=0o700)
saved = sys.argv[:]
try:
    sys.argv = [str(here / 'build_qwen38_q4_constant_issue_worker_image.py'), str(sdk), str(out / 'unchanged-constant-q4')]
    parent = runpy.run_path(sys.argv[0], run_name='__main__')
finally:
    sys.argv = saved
control = out / 'unchanged-constant-q4'
assert hashlib.sha256((control / 'program.elf').read_bytes()).hexdigest() == 'e9ef5c53dbb3f8a924920c5dac261a884bbe33c3168d90cfd168e02fa96d8397'
tool = parent['tool']
command = (control / 'compile-math-argv.txt').read_text().splitlines()
original = command[command.index('-o') + 1]
matrix = out / 'manual-ttsync-program.o'
command[command.index('-c') + 1] = str(here / 'blackhole_qwen38_manual_ttsync_math.cpp')
command[command.index('-o') + 1] = str(matrix)
command = [command[0], '-DPIONEER_Q38_MANUAL_TTSYNC_IMAGE=1', *command[1:]]
subprocess.run(command, check=True)
(out / 'compile-math-argv.txt').write_text('\n'.join(command) + '\n')
script = (control / 'program.ld').read_text()
needle = '\t\t*(.q38_projection_data)\n'
assert script.count(needle) == 1
# Preserve the ORIGINAL e9ef constant-data ABI at71f20. The shorter wait
# can reduce preceding helper code;do not move tables referenced by the
# otherwise byte-identical FFN/GDN/service capsules. A helper that grows
# past this fixed offset fails the linker rather than widening the view.
script = script.replace(needle, '\t\t. = 0x1420; /* original projection-data offset in affine */\n' + needle)
needle = '\t\tKEEP(*(.text.start)) *(.text .text.* .rodata .rodata.* .srodata .srodata.*)\n'
assert script.count(needle) == 1
# The original Drain occupies72000..72057. Keep its now-shorter body in
# that SAME slot and preserve the immediately following entry. Later new
# math internals still need their own instruction/data qualification. No
# runtime NOPs are inserted:the new RET precedes this unreachable padding.
script = script.replace(needle, '\t\tKEEP(*(.text.start)) *(.text._ZL5Drainv)\n'
                        '\t\t. = 0x58; /* original Drain slot;not a larger extent */\n'
                        '\t\t*(.text .text.* .rodata .rodata.* .srodata .srodata.*)\n')
(out / 'program.ld').write_text(script)
link = (control / 'link-argv.txt').read_text().splitlines()
assert link.count(original) == 1
link = [str(matrix) if value == original else value for value in link]
for option, name in (('-T', 'program.ld'), ('-Map', 'program.map'), ('-o', 'program.elf')):
    link[link.index(option) + 1] = str(out / name)
subprocess.run(link, check=True)
(out / 'link-argv.txt').write_text('\n'.join(link) + '\n')
elf = out / 'program.elf'
assert not subprocess.check_output([tool + 'nm', '-u', str(elf)]).strip()
symbols = subprocess.check_output([tool + 'nm', '-n', str(elf)], text=True)
names = subprocess.check_output([tool + 'nm', '-nC', str(elf)], text=True)
assert not re.search(r'^[0-9a-f]+ [bBdDgGsS] ', symbols, re.M)
for name in ('PioneerQ38Q4ConstantTransform<', 'TwinBatchTilePrefetchStage(', 'q38_compute_once'):
    assert name in names, name
dis = subprocess.check_output([tool + 'objdump', '-d', str(elf)], text=True)
assert not re.search(r'\b(?:divu?|remu?|mulh[su]*|fdiv\.[sd]|fsqrt\.[sd]|fmadd|fmsub|fnmadd|fnmsub|fcvt\.wu?\.s|ebreak)\b', dis)
frames = [row.split('\t') for row in matrix.with_suffix('.su').read_text().splitlines()]
assert frames and all(len(r) == 3 and r[2] == 'static' and int(r[1]) <= 4096 for r in frames)
sections = parent['sections']
rows = subprocess.check_output([tool + 'objdump', '-h', str(elf)], text=True).splitlines()
for i, row in enumerate(rows):
    m = re.match(r'\s*\d+\s+(\S+)\s+([0-9a-f]+)\s+', row)
    if m and 'ALLOC' in rows[i + 1]:
        assert m[1][1:] in sections, row
for section in sections:
    subprocess.run([tool + 'objcopy', '-O', 'binary', '-j', '.' + section, str(elf), str(out / (section + '.bin'))], check=True)
for section, limit in (('text', 0x2000), ('affine', 0x1500), ('fetch', 0x700), ('service', 0x6000)):
    assert 0 < (out / (section + '.bin')).stat().st_size <= limit, section
for section in ('boot', 'globalworker', 'precise', 'dense', 'ffn', 'service', 'gdn'):
    assert (out / (section + '.bin')).read_bytes() == (control / (section + '.bin')).read_bytes(), section
# Math text/affine are deliberately NEW implementation bytes and need
# their own instruction/lifetime tests. Only two fixed immutable lookup
# pointers may relocate in the otherwise unchanged fetch helper.
raw = [{s: (d / (s + '.bin')).read_bytes() for s in sections} for d in (control, out)]
objects = []
for directory in (control, out):
    table = {}
    for row in subprocess.check_output([tool + 'readelf', '-sW', str(directory / 'program.elf')], text=True).splitlines():
        f = row.split()
        if len(f) == 8 and f[0].endswith(':') and f[3] == 'OBJECT' and int(f[2]):
            name = re.sub(r'^CSWTCH\.\d+$', 'CSWTCH', f[7])
            table[int(f[1], 16)] = (name, int(f[2]))
    objects.append(table)
fetch = [struct.unpack('<' + 'I' * (len(r['fetch']) // 4), r['fetch']) for r in raw]
assert len(fetch[0]) == len(fetch[1]) == 323
allowed = set()
for high_pc, low_pc in ((0x701d8, 0x701e0), (0x70424, 0x70428)):
    hi, lo = (high_pc - 0x70000) // 4, (low_pc - 0x70000) // 4
    addresses = []
    for words in fetch:
        upper, lower = words[hi], words[lo]
        reg = (upper >> 7) & 31
        assert upper & 127 == 0x37 and lower & 0x707f == 0x13
        assert (lower >> 7) & 31 == (lower >> 15) & 31 == reg and reg
        signed_low = (lower >> 20) - (4096 if lower >> 31 else 0)
        addresses.append(((upper & 0xfffff000) + signed_low) & 0xffffffff)
    assert fetch[0][hi] & 0xfff == fetch[1][hi] & 0xfff
    assert fetch[0][lo] & 0xfffff == fetch[1][lo] & 0xfffff
    rows = [objects[i][address] for i, address in enumerate(addresses)]
    assert rows[0] == rows[1] == ('CSWTCH', 28)
    data = [raw[i]['text'][address - 0x72000:address - 0x72000 + 28] for i, address in enumerate(addresses)]
    assert all(0x72000 <= address <= 0x74000 - 28 for address in addresses)
    assert len(data[0]) == 28 and data[0] == data[1]
    allowed.update((hi, lo))
    print('AUDITED_IDENTICAL_MANUAL_COMPLETION_FETCH_DATA', hex(high_pc), hex(low_pc), rows[0], flush=True)
assert {i for i, (a, b) in enumerate(zip(*fetch)) if a != b} <= allowed
# Audit the COMPLETE new routine,not disassembly comments or host labels.
expected = (0xffe807b7, 0x00478793, 0x0007a023, 0x0007a703,
            0x00077713, 0x0ff0000f, 0x00008067)
assert struct.unpack('<7I', raw[1]['text'][:28]) == expected
assert raw[1]['text'][28:0x58] == bytes(0x58 - 28)
(out / 'program.symbols').write_text(symbols)
(out / 'program.disassembly').write_text(dis)
subprocess.run([sys.executable, str(here / 'tests/pioneer_q38_gdn_rendezvous_csr_gate.py'), str(out / 'program.disassembly')], check=True)
observed = []
for name in (*(s + '.bin' for s in sections), 'program.elf'):
    data = (out / name).read_bytes()
    assert data and len(data) % 4 == 0
    item = (name, len(data), hashlib.sha256(data).hexdigest())
    observed.append(item)
    print(item, flush=True)
if not inspect:
    assert tuple(observed) == PINS
print('BUILD_ONLY complete returning manual completion prototype/ENTIRE olde9ef FIRST/'
      'ordered discarded store+blocking load+dependent ANDI+fence;NO native,lifecycle,physical,TPS,quality or Work grant', flush=True)
