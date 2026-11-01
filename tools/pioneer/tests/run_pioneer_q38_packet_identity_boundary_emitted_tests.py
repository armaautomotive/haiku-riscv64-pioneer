#!/usr/bin/env python3
"""Actual emitted bodies/call sites ONLY;not path,model or device timing."""
import hashlib
import os
import pathlib
import re
import runpy
import struct
import subprocess
import sys

assert len(sys.argv) == 3 and not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
old, new = map(pathlib.Path, sys.argv[1:])
here = pathlib.Path(__file__).resolve().parent
saved = sys.argv[:]
try:
    retained = old / 'unchanged-commit/unchanged-retained'
    sys.argv = [str(here / 'run_pioneer_q38_retained_conv_emitted_tests.py'),
                str(retained / 'unchanged-frontier'), str(retained)]
    audit = runpy.run_path(sys.argv[0], run_name='__main__')
finally:
    sys.argv = saved
for directory, script in ((old, 'build_qwen38_cohort_local_sources_root.py'),
                          (new, 'build_qwen38_packet_identity_boundary_root.py')):
    pins = audit['literal'](here.parent / script, 'PINS')
    assert directory.is_absolute() and not directory.is_symlink() and pins
    for name, size, digest in pins:
        data = (directory / name).read_bytes()
        assert len(data) == size and hashlib.sha256(data).hexdigest() == digest, (directory, name)


def objects(directory):
    result = []
    for line in subprocess.check_output([audit['tool'] + 'objdump', '-tC', str(directory / 'program.elf')], text=True).splitlines():
        row = line.split(None, 5)
        if len(row) != 6 or row[2] != 'O' or row[3] not in ('.text', '.conv'):
            continue
        address, size = int(row[0], 16), int(row[4], 16)
        base = 0x40000 if row[3] == '.text' else 0x64000
        data = (directory / ('text.bin' if row[3] == '.text' else 'conv.bin')).read_bytes()
        raw = data[address - base:address - base + size]
        assert size and len(raw) == size
        result.append((address, size, re.sub(r'CSWTCH\.\d+', 'CSWTCH', row[5]), raw))
    return result


def bodies(directory, prefix):
    table = audit['functions'](directory)
    constants = objects(directory)
    result = []
    for start, (size, name) in sorted(table.items()):
        if not name.startswith(prefix):
            continue
        base = 0x40000 if start < 0x60000 else 0x64000
        data = (directory / ('text.bin' if base == 0x40000 else 'conv.bin')).read_bytes()
        raw = data[start - base:start - base + size]
        assert size and len(raw) == size and not size & 3
        words = list(struct.unpack('<' + 'I' * (size // 4), raw))
        # LUI+ADDI materializations of real read-only OBJECTS move with the
        # shorter text. Retain register/opcode bits and record each exact
        # object name,size,ALL literal bytes and intra-object offset. No
        # arithmetic immediates or unknown address targets are normalized.
        high = {}
        literals = []
        for index, word in enumerate(words[:]):
            opcode, rd, rs1 = word & 127, (word >> 7) & 31, (word >> 15) & 31
            if opcode == 0x37:
                high[rd] = (index, word & 0xfffff000)
                continue
            if opcode == 0x13 and not (word >> 12) & 7 and rs1 in high:
                first, value = high[rs1]
                address = (value + audit['sign'](word >> 20, 12)) & 0xffffffff
                found = {(name, size, data, address - start) for start, size, name, data in constants
                         if start <= address < start + size}
                if found:
                    assert len(found) == 1, (name, hex(address), 'ambiguous literal identity')
                    literals.append((first, index, next(iter(found))))
                    words[first] &= 0xfff
                    words[index] &= 0xfffff
            if rd and opcode in (3, 7, 0x13, 0x17, 0x33, 0x53, 0x67, 0x6f):
                high.pop(rd, None)
        calls = []
        for index, word in enumerate(words[:]):
            pc, opcode = start + index * 4, word & 127
            rd, target = (word >> 7) & 31, None
            if opcode == 0x6f:
                immediate = ((word >> 31) << 20) | (((word >> 21) & 1023) << 1) \
                    | (((word >> 20) & 1) << 11) | (((word >> 12) & 255) << 12)
                target = (pc + audit['sign'](immediate, 21)) & 0xffffffff
                if not rd and start <= target < start + size:
                    continue
                assert rd in (0, 1)
                assert target in table
                words[index] &= 0xfff  # LINK displacement only;retain rd/opcode.
            elif opcode == 0x67:
                rs1, immediate = (word >> 15) & 31, audit['sign'](word >> 20, 12)
                if not rd and rs1 == 1 and not immediate:
                    continue  # Actual ABI return.
                assert index and rd in (0, 1)
                prior = words[index - 1]
                assert prior & 127 == 0x17 and (prior >> 7) & 31 == rs1, (name, hex(pc), 'unresolved indirect jump')
                target = (pc - 4 + audit['sign'](prior & 0xfffff000, 32) + immediate) & 0xfffffffe
                assert target in table, (name, hex(pc), hex(target))
                words[index - 1] &= 0xfff
                words[index] &= 0xfffff
            if target is not None:
                calls.append((pc - start, table[target][1]))
        result.append((name, words, calls, literals))
    assert result, prefix
    return result


names = ('DensePartitionRoot::Transfer(', 'MixedRecurrentCohort::Packet(',
         'RecurrentBlock::Norm(', 'RecurrentBlock::Matrix(',
         'RecurrentBlock::Cohort(', 'RecurrentBlock::Finish(', 'RecurrentBlock::Transfer(',
         'RecurrentBlock::Read(', 'RecurrentBlock::EndSeal(', 'RecurrentBlock::CaptureRecipes(',
         'RecurrentBlock::RecipeSeal(',
         'q38_return_recurrent_layer', 'GlobalChain::CommitState(', 'GlobalChain::StateSeal(',
         'GlobalChain::InputSeal(', 'GlobalChain::FeedbackSeal(', 'GlobalChain::Packet(',
         'GlobalChain::Transfer(', 'GlobalChain::Scope(')
count = 0
for name in names:
    before, after = bodies(old, name), bodies(new, name)
    if before != after:
        for ordinal, (a, b) in enumerate(zip(before, after)):
            print('BODY_DIFFERENCE', name, ordinal, 'words', len(a[1]), len(b[1]), 'callsEqual', a[2] == b[2], flush=True)
            print('WORDS', [(i, hex(x), hex(y)) for i, (x, y) in enumerate(zip(a[1], b[1])) if x != y][:16], flush=True)
    assert before == after, ('body+named call-target changes beyond LINK relocation', name)
    count += len(before)
packet = 'q38_layer_return_recurrent_packet_seal'
full = 'q38_layer_return_recurrent_seal'
identity = 'RecurrentBlock::PacketIdentity('
for directory in (old, new):
    assert not any(target.startswith(identity) for _, _, calls, _ in bodies(directory, packet) for _, target in calls)
    calls = [target for _, _, calls, _ in bodies(directory, full) for _, target in calls]
    assert sum(target.startswith(identity) for target in calls) == int(directory == new)
assert len(audit['unresolved_jumps']) == 2  # Only inherited,explicit StageConv switches.
print(f'PASS actual emitted cd54->0c3f/{count} transfer,worker-release,compute,commit and feedback '
      'bodies:every instruction+named direct call target EXACT except decoded LINK and authenticated read-only literal-address displacements;'
      'new full seal adds exactly1 PacketIdentity call site,packet adds0;'
      'Projection::Run indexed switch NOT covered by this direct-body audit;'
      'STATIC ONLY,not branch feasibility,genuine owner,model,native,physical,Work or TPS proof')
