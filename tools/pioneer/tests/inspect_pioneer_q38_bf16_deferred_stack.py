#!/usr/bin/env python3
"""Conservative stack bound for the exact deferred-audit BUILD; no execution.

The old profile inspector is deliberately unchanged. All nine switches below
were inspected anew in this ELF, including table construction, unsigned index
guards and literal targets. This does not qualify a native launch or hardware.
"""
import bisect
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys

assert __debug__ and len(sys.argv) == 3
sdk, elf = map(Path, sys.argv[1:])
assert sdk.is_absolute() and elf.is_absolute()
tool = str(sdk / 'cross-tools-riscv64/bin/riscv64-unknown-haiku-objdump')
digest = hashlib.sha256(elf.read_bytes()).hexdigest()
assert digest == 'd065a29672996db27471e19e0cc0f396eea32242043d7e89a95f197ac25e9fb5'
# pc: (immutable table base, entries, unsigned index guard pc).
# Two Run() tables are initialized at51538..51558 and52984..52990,
# then loaded from the same -808(s0) slot. RunHead's unchanged table is
# initialized at66030..66048 and loaded from -1508(s0). No arbitrary
# indirect target, unknown table, or compiler revision is accepted here.
switches = {
    0x23550: (0x5f2dc, 8, 0x23540),
    0x264f4: (0x5f3e0, 8, 0x264e4),
    0x273a8: (0x5f444, 9, 0x2739c),
    0x4faa8: (0x5f468, 9, 0x4fa98),
    0x518f8: (0x5f48c, 9, 0x518e4),
    0x52d28: (0x5f4b0, 9, 0x52d14),
    0x55bd0: (0x5f4d4, 9, 0x55bc0),
    0x5bd68: (0x5f674, 9, 0x5bd58),
    0x6644c: (0x4657c, 9, 0x66438),
}
raw = {}
for row in subprocess.check_output(
        [tool, '-s', '-j', '.text', '-j', '.conv', '-j', '.bf16', str(elf)],
        text=True).splitlines():
    fields = row.split()
    if fields and re.fullmatch('[0-9a-f]{5,8}', fields[0]):
        address = int(fields[0], 16)
        for word in fields[1:5]:
            if not re.fullmatch('[0-9a-f]{8}', word):
                break
            raw[address] = int.from_bytes(bytes.fromhex(word), 'little')
            address += 4
functions = {}
for row in subprocess.check_output([tool, '-t', str(elf)], text=True).splitlines():
    fields = row.split()
    if len(fields) == 6 and fields[2] == 'F' and int(fields[4], 16):
        address, size = int(fields[0], 16), int(fields[4], 16)
        if address in functions:
            assert functions[address]['size'] == size
            functions[address]['name'] += '|' + fields[5]
        else:
            functions[address] = dict(name=fields[5], size=size, frame=0, edges=set())
for address, name, size in (
        (0x6000, '_start', 4), (0x40000, 'scalar_entry', 16),
        (0x60000, 'scalar_fault_park', 4), (0x60004, 'q38_global_chain_stopped', 4)):
    if address not in functions:
        functions[address] = dict(name=name, size=size, frame=0, edges=set())
starts = sorted(functions)


def owner(pc):
    index = bisect.bisect_right(starts, pc) - 1
    assert index >= 0, hex(pc)
    address = starts[index]
    assert pc - address < functions[address]['size'], hex(pc)
    return address


instructions = {
    int(m[1], 16): int(m[2], 16) for m in re.finditer(
        r'^\s*([0-9a-f]+):\s+([0-9a-f]{8})\s',
        subprocess.check_output([tool, '-d', str(elf)], text=True), re.M)}


def signed(value, bits):
    return value - (1 << bits) if value >> (bits - 1) else value


def edge(caller, target):
    callee = owner(target)
    if callee != caller:
        functions[caller]['edges'].add(callee)


unknown, seen, calls = [], set(), 0
for address, info in functions.items():
    for pc in range(address, address + info['size'], 4):
        assert pc in instructions, (info['name'], hex(pc))
        inst = instructions[pc]
        op, dest, source, fun = inst & 127, (inst >> 7) & 31, (inst >> 15) & 31, (inst >> 12) & 7
        if dest == 2 and op in (0x13, 0x33, 0x03, 0x37, 0x17, 0x67, 0x6f):
            if op == 0x13 and source == 2 and fun == 0:
                info['frame'] += max(0, -signed(inst >> 20, 12))
            elif op == 0x37 and pc == 0x40000:
                assert inst == 0x00070137
            else:
                unknown.append(('stack', hex(pc), hex(inst), info['name']))
        if op == 0x67:
            imm = signed(inst >> 20, 12)
            if dest == 0 and source == 1 and imm == 0:
                continue
            prior = instructions.get(pc - 4, 0)
            if prior & 127 == 0x17 and (prior >> 7) & 31 == source:
                edge(address, ((pc - 4) + signed(prior & 0xfffff000, 32) + imm) & 0xfffffffe)
                calls += 1
            elif pc in switches:
                base, count, guard = switches[pc]
                assert dest == 0 and imm == 0 and prior & 127 == 3
                assert instructions[guard] & 0x707f == 0x6063  # unsigned greater-than -> default
                for index in range(count):
                    assert owner(raw[base + index * 4]) == address, (hex(pc), index)
                seen.add(pc)
            else:
                unknown.append(('indirect', hex(pc), hex(inst), info['name']))
        elif op == 0x6f:
            imm = ((inst >> 31) << 20) | (((inst >> 21) & 1023) << 1) | (((inst >> 20) & 1) << 11) | (((inst >> 12) & 255) << 12)
            edge(address, pc + signed(imm, 21))
            calls += dest != 0
        elif op == 0x63:
            imm = ((inst >> 31) << 12) | (((inst >> 25) & 63) << 5) | (((inst >> 8) & 15) << 1) | (((inst >> 7) & 1) << 11)
            edge(address, pc + signed(imm, 13))
assert not unknown, unknown
assert seen == switches.keys()
cache = {}


def bound(address, visiting=()):
    assert address not in visiting, ('call cycle', [hex(x) for x in (*visiting, address)])
    if address in cache:
        return cache[address]
    best = max((bound(child, (*visiting, address)) for child in functions[address]['edges']),
               default=(0, []), key=lambda result: result[0])
    result = functions[address]['frame'] + best[0], [address, *best[1]]
    cache[address] = result
    return result


total, path = bound(0x6000)
assert total <= 8192, ('whole-ROOT stack exceeds reservation', total)
print(json.dumps(dict(
    elf_sha256=digest, conservativeBytes=total, stackBytes=8192,
    functions=len(functions), calls=calls, switches=len(seen),
    path=[dict(address=hex(address), name=functions[address]['name'], frame=functions[address]['frame'])
          for address in path],
    scope='sum of all negative SP immediates/function plus direct call/tail/branch callees; fixed inspected intra-function switches; NOT execution/native/hardware/TPS'), indent=2))
