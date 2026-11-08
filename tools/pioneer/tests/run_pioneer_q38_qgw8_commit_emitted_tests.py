#!/usr/bin/env python3
"""Audit pinned linked QGW8 RV32 call edges; no card or execution claim."""
import hashlib
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile

assert len(sys.argv) == 3 and not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
old, new = map(Path, sys.argv[1:])
pins = ((old, '8b7bac64e64d963e729e347a209c3ca4be951992aa924ead0c48d97e0e79478f'),
        (new, '9ee84a211ba8a0acb13aff818711f679dd1b88ba4be9a7f385a4c0f60377d401'))
tool = '/Volumes/HaikuBuildLocal/generated.riscv64/cross-tools-riscv64/bin/riscv64-unknown-haiku-'
for directory, digest in pins:
    assert directory.is_absolute() and directory.is_dir() and not directory.is_symlink()
    assert hashlib.sha256((directory / 'program.elf').read_bytes()).hexdigest() == digest
    with tempfile.TemporaryDirectory(prefix='qgw8-commit-emitted-', dir='/private/tmp') as tmp:
        for section in ('boot', 'text', 'terminal', 'conv', 'bf16'):
            path = Path(tmp) / (section + '.bin')
            subprocess.run([tool + 'objcopy', '-O', 'binary', '-j', '.' + section,
                            str(directory / 'program.elf'), str(path)], check=True)
            assert path.read_bytes() == (directory / (section + '.bin')).read_bytes()
for section in ('boot', 'terminal', 'bf16'):
    assert (old / (section + '.bin')).read_bytes() == (new / (section + '.bin')).read_bytes()

def sign(value, bits):
    return value - (1 << bits) if value & (1 << (bits - 1)) else value

def functions(directory):
    result = {}
    rows = subprocess.check_output([tool + 'nm', '-S', '-nC', '--defined-only', str(directory / 'program.elf')], text=True)
    for line in rows.splitlines():
        row = line.split(None, 3)
        if len(row) == 4 and row[2] in ('t', 'T', 'w', 'W'):
            start, size = int(row[0], 16), int(row[1], 16)
            if size:
                result[start] = (size, row[3])
    plain = subprocess.check_output([tool + 'nm', '-nC', '--defined-only', str(directory / 'program.elf')], text=True)
    for address, name in ((0x6000, '_start'), (0x40000, 'scalar_entry'),
                          (0x60000, 'scalar_fault_park'), (0x60004, 'q38_global_chain_stopped')):
        assert any(line == f'{address:08x} T {name}' for line in plain.splitlines())
        result.setdefault(address, (0, name))
    return result

def data_for(directory, start, size):
    if 0x20000 <= start < 0x30000:
        base, section = 0x20000, 'bf16'
    elif 0x40000 <= start < 0x60000:
        base, section = 0x40000, 'text'
    else:
        assert 0x64000 <= start < 0x6c000
        base, section = 0x64000, 'conv'
    data = (directory / (section + '.bin')).read_bytes()
    assert size and not size & 3 and start - base + size <= len(data)
    return data[start - base:start - base + size]

def body(directory, needle, normalize=False):
    table = functions(directory)
    matches = [(start, size, name) for start, (size, name) in table.items() if name.startswith(needle)]
    assert len(matches) == 1, (needle, matches)
    start, size, name = matches[0]
    raw = data_for(directory, start, size)
    words = list(struct.unpack('<' + 'I' * (size // 4), raw))
    calls = []
    for index, word in enumerate(words):
        pc, opcode, rd, target = start + index * 4, word & 127, (word >> 7) & 31, None
        if opcode == 0x6f:
            immediate = ((word >> 31) << 20) | (((word >> 21) & 1023) << 1) | (((word >> 20) & 1) << 11) | (((word >> 12) & 255) << 12)
            target = (pc + sign(immediate, 21)) & 0xffffffff
            if not rd and start <= target < start + size:
                continue
            assert rd in (0, 1)
        elif opcode == 0x67:
            rs1, immediate = (word >> 15) & 31, sign(word >> 20, 12)
            if rd == 0 and rs1 == 1 and not immediate:
                continue
            assert index and rd in (0, 1)
            previous = words[index - 1]
            assert previous & 127 == 0x17 and ((previous >> 7) & 31) == rs1, (name, hex(pc))
            target = (pc - 4 + sign(previous & 0xfffff000, 32) + immediate) & 0xfffffffe
        if target is not None:
            assert target in table, (name, hex(pc), hex(target))
            calls.append((pc - start, table[target][1]))
            if normalize:
                if opcode == 0x6f:
                    words[index] &= 0xfff
                else:
                    words[index - 1] &= 0xfff
                    words[index] &= 0xfffff
    return (words if normalize else raw), calls

prefix = 'GlobalChain::'
def count(calls, target):
    return sum(name.startswith(prefix + target + '(') for _, name in calls)

before = body(old, prefix + 'StateSeal(')[1]
after = body(new, prefix + 'StateSeal(')[1]
assert (count(before, 'InputSeal'), count(before, 'FeedbackSeal'), count(before, 'HeadSeal')) == (1, 1, 1)
assert (count(after, 'InputSeal'), count(after, 'FeedbackSeal'), count(after, 'HeadSeal')) == (0, 0, 1)
before = body(old, prefix + 'CommitState(')[1]
after = body(new, prefix + 'CommitState(')[1]
assert (count(before, 'InputSeal'), count(before, 'FeedbackSeal')) == (1, 1)
assert (count(after, 'InputSeal'), count(after, 'FeedbackSeal')) == (2, 2)
for target, expected in (('StateSeal', 4), ('Packet', 7), ('Transfer', 1)):
    assert count(before, target) == count(after, target) == expected
for target in ('Packet', 'Transfer', 'HeadSeal', 'InputSeal', 'FeedbackSeal', 'Scope'):
    assert body(old, prefix + target + '(', True) == body(new, prefix + target + '(', True), target
assert body(old, prefix + 'Packet(')[0] == body(new, prefix + 'Packet(')[0]
print('PASS pinned QGW8 RV32 call sites, packet bytes, and retained guard instructions/callees; '
      'static only: NO runtime, numerical, native, physical or TPS qualification')
