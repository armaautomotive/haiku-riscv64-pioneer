#!/usr/bin/env python3
"""Isolated current-QGW3 prefetch shader BUILD only;NEVER install or run."""
import hashlib
import os
import pathlib
import re
import runpy
import struct
import subprocess
import sys

PINS = (
    ('boot.bin',4,'a12af9d63593a1f882750a147dd7a511ac476c5c7a55526bd4d51c685173991c'),
    ('globalworker.bin',9128,'ede241b1a22fe541acd6a89470b8a93ad9d5e576127965745a3a932371befac1'),
    ('precise.bin',8392,'32c078029e1c74ec4bd9744ea494433f5003f63a936df05445d3066c722a7ecc'),
    ('dense.bin',10332,'d1749707ffe803e28dced9c7d065e763f125009678ace33b4d468109984869ad'),
    ('fetch.bin',856,'9529379ea2277127520adf49fe6b17b7c37b701ed3108b1fed1ac08a7ddef561'),
    ('text.bin',5948,'530e72df699db2c6d16b8360115d8f94cdbb6f542ed2f23653c59f75a266dce8'),
    ('affine.bin',5360,'ec3428e6f946b113e7e39575df8c768dc2f6e9a56b79c47dad16c283c12ee159'),
    ('ffn.bin',16456,'23ce6cc2dc07aa445b7babcac12a068dc3542001329cd24831e8e6749ff3a16f'),
    ('service.bin',9888,'7d5a42a087bc6bf9d24319583f3655ae344828a51b1092154f7b0567b0fdb187'),
    ('gdn.bin',35408,'cbe3faf60ef0b3a0544b27b29f99d62eb36c7d0ac5b835e337c5501661ed589c'),
    ('program.elf',143756,'cf02c9777314443510894903a74637277be1ccc9b0db8ae80e7fd900baea7120'),
)  # Independent main/clean BUILD identity ONLY;NO runtime/native permission.
args = sys.argv[1:]
inspect = '--inspect-candidate' in args
if inspect:
    assert args.count('--inspect-candidate') == 1
    args.remove('--inspect-candidate')
assert len(args) == 2 and (inspect or PINS)
assert not any(key.startswith(('PIONEER_', 'TT_PIONEER_')) for key in os.environ)
sdk, out = map(pathlib.Path, args)
here = pathlib.Path(__file__).resolve().parent
assert sdk.is_absolute() and out.is_absolute() and not out.exists() and not out.is_symlink()
out.mkdir(mode=0o700)
saved = sys.argv[:]
try:
    sys.argv = [str(here / 'build_qwen38_precise_beta_worker_image.py'), str(sdk), str(out / 'unchanged-beta')]
    old = runpy.run_path(sys.argv[0], run_name='__main__')
finally:
    sys.argv = saved
control = out / 'unchanged-beta'
assert hashlib.sha256((control / 'program.elf').read_bytes()).hexdigest() == '1f49e9740d8d9045de6c8835f0f8f1738210ba0413d87f607f97cf98da74539e'
ancestor = old['ancestor']
mixed = ancestor['old']['parent']['old']
unit = next(item for item in mixed['units'] if item[1] == 'program')
assert unit[0] == 'blackhole_qwen38_resident_operator_service.cpp' and unit[3] == '-O2'
tool = str(ancestor['cross']) + '-'
precise = control / 'unchanged-pushed/unchanged-precise'
math = precise / 'unchanged-global/qualified-math'


def run(*values):
    subprocess.run(list(map(str, values)), check=True)


command = [tool + 'g++', *ancestor['flags'], unit[3], *mixed['fixed'], *unit[2],
           '-c', str(here / unit[0]), '-o']
original = out / 'unchanged-program.o'
run(*command, original)
assert original.read_bytes() == (math / 'program.o').read_bytes(), 'ENTIRE old matrix/service object must reproduce FIRST'
obj = out / 'prefetch-program.o'
new_command = [command[0], '-DPIONEER_Q38_TOKEN_PREFETCH_WORKER_IMAGE=1',
               '-DPIONEER_Q38_WORK_TILE_PREFETCH=1', *command[1:], str(obj)]
run(*new_command)
(out / 'compile-argv.txt').write_text('\n'.join(new_command) + '\n')
objects = [obj if path == math / 'program.o' else path for path in old['objects']]
assert objects.count(obj) == 1 and objects.count(control / 'gdn_input.o') == 1
(out / 'program.ld').write_bytes((control / 'program.ld').read_bytes())
elf = out / 'program.elf'
link = [tool + 'ld', '-m', 'elf32lriscv', '--no-relax', '--gc-sections', '-T', str(out / 'program.ld'),
        str(precise / 'start.o'), str(control / 'unchanged-pushed/global.o'), str(precise / 'precise.o'),
        *map(str, objects), '-Map', str(out / 'program.map'), '-o', str(elf)]
run(*link)
(out / 'link-argv.txt').write_text('\n'.join(link) + '\n')
symbols = subprocess.check_output([tool + 'nm', '-n', str(elf)], text=True)
demangled = subprocess.check_output([tool + 'nm', '-nC', str(elf)], text=True)
assert not subprocess.check_output([tool + 'nm', '-u', str(elf)], text=True).strip()
assert not re.search(r'^[0-9a-f]+ [bBdDgGsS] ', symbols, re.M), 'no writable globals/GOT'
for name in ('TilePrefetchStart(', 'TilePrefetchWait(', 'TilePrefetchStage(', 'q38_compute_once',
             'q38_mixed_seal', 'q38_global_worker_main', 'q38_precise_persistent_activation'):
    assert name in demangled, name
disassembly = subprocess.check_output([tool + 'objdump', '-d', str(elf)], text=True)
assert not re.search(r'\b(?:divu?|remu?|mulh[su]*|fdiv\.[sd]|fsqrt\.[sd]|fmadd|fmsub|fnmadd|fnmsub|fcvt\.wu?\.s|ebreak)\b', disassembly)
reserved = [line for line in disassembly.splitlines() if re.search(r'\bs11\b', line)]
assert len(reserved) == 2 and sum(bool(re.search(r'\bmv\s+s11,\w+\s*$', line)) for line in reserved) == 1
assert sum(bool(re.search(r'\bmv\s+\w+,s11\s*$', line)) for line in reserved) == 1
frames = [int(line.split('\t')[1]) for line in obj.with_suffix('.su').read_text().splitlines()]
assert frames and max(frames) <= 4096, max(frames)
sections = ('boot', 'globalworker', 'precise', 'dense', 'fetch', 'text', 'affine', 'ffn', 'service', 'gdn')
rows = subprocess.check_output([tool + 'objdump', '-h', str(elf)], text=True).splitlines()
for index, line in enumerate(rows):
    match = re.match(r'\s*\d+\s+(\S+)\s+([0-9a-fA-F]+)\s+', line)
    if match and 'ALLOC' in rows[index + 1]:
        assert match[1][1:] in sections, line
for section in sections:
    run(tool + 'objcopy', '-O', 'binary', '-j', '.' + section, elf, out / (section + '.bin'))
assert (out / 'boot.bin').read_bytes() == (control / 'boot.bin').read_bytes()
assert 0 < (out / 'fetch.bin').stat().st_size <= 0x700
assert 0 < (out / 'affine.bin').stat().st_size <= 0x1500
assert 0 < (out / 'text.bin').stat().st_size <= 0x2000
for section in ('precise', 'dense', 'ffn', 'service', 'gdn'):
    assert (out / (section + '.bin')).read_bytes() == (control / (section + '.bin')).read_bytes(), section


def functions(path):
    result = {}
    for line in subprocess.check_output([tool + 'nm', '-S', '--defined-only', str(path)], text=True).splitlines():
        row = line.split()
        if len(row) == 4 and row[2] in ('t', 'T', 'w', 'W'):
            result[int(row[0], 16)] = (row[3], int(row[1], 16))
    return result


# The entire old global.o is linked unchanged. Audit EVERY linked byte
# difference,not a blanket claim that non-matrix sections stayed identical.
# Only calls to the INTENTIONALLY replaced q38_compute_once may relocate.
old_front = (control / 'globalworker.bin').read_bytes()
new_front = (out / 'globalworker.bin').read_bytes()
assert len(old_front) == len(new_front) == 9128
words = (struct.unpack('<2282I', old_front), struct.unpack('<2282I', new_front))
changed = {index for index in range(2282) if words[0][index] != words[1][index]}
maps = (functions(control / 'program.elf'), functions(elf))
covered = set()
for index in range(2281):
    if not changed.intersection((index, index + 1)):
        continue
    if not all(part[index] & 0xfff == 0x097 and part[index + 1] & 0xfffff == 0x080e7 for part in words):
        continue
    targets = []
    for side, part in enumerate(words):
        high = part[index] & 0xfffff000
        high -= (1 << 32) if high >> 31 else 0
        low = part[index + 1] >> 20
        low -= 4096 if low >> 11 else 0
        address = 0x20000 + index * 4 + high + low
        name, size = maps[side][address]
        assert name == 'q38_compute_once' and size, (hex(address), name, size)
        targets.append(address)
    covered.update(changed.intersection((index, index + 1)))
    print('AUDITED_CURRENT_MATRIX_CALL_RELOCATION', hex(0x20000 + index * 4), *map(hex, targets), flush=True)
assert changed and changed == covered, (changed, covered)
(out / 'program.symbols').write_text(symbols)
(out / 'program.disassembly').write_text(disassembly)
run(sys.executable, here / 'tests/pioneer_q38_gdn_rendezvous_csr_gate.py', out / 'program.disassembly')
observed = []
for name in (*(section + '.bin' for section in sections), 'program.elf'):
    data = (out / name).read_bytes()
    assert data and not len(data) % 4
    item = (name, len(data), hashlib.sha256(data).hexdigest())
    observed.append(item)
    print(item, flush=True)
if not inspect:
    assert tuple(observed) == PINS, 'new subject is not independently frozen'
print('BUILD_ONLY distinct full-model prefetch shader/ENTIRE old precise-beta+ancestors+matrix object FIRST/'
      'original section limits/individual frames bounded;'
      'NO emitted execution,numeric,state,lifetime,native,hardware,Work or performance qualification', flush=True)
