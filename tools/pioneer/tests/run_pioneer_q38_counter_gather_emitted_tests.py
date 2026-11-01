#!/usr/bin/env python3
"""Selected emitted-body audit, not model/lifecycle/hardware performance."""
import hashlib
import os
import pathlib
import runpy
import sys
import struct
from collections import Counter

assert len(sys.argv) == 2 and not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
new = pathlib.Path(sys.argv[1])
old = new / 'unchanged-tid'
here = pathlib.Path(__file__).resolve().parent
saved = sys.argv[:]
try:
    sys.argv = [str(here / 'run_pioneer_q38_tid_boundary_emitted_tests.py'), str(old)]
    proof = runpy.run_path(sys.argv[0], run_name='__main__')
finally:
    sys.argv = saved
assert new.is_absolute() and not new.is_symlink()
pins = proof['proof']['audit']['literal'](here.parent / 'build_qwen38_counter_gather_root.py', 'PINS')
assert pins
for name, size, digest in pins:
    data = (new / name).read_bytes()
    assert len(data) == size and hashlib.sha256(data).hexdigest() == digest, name
bodies = proof['bodies']


def scalar_literals(directory, rows):
    # The inherited normalizer recognizes LUI+ADDI object addresses. GCC
    # also uses LUI+LW/FLW for four-byte scalar pools. Normalize ONLY a
    # decoded read of immutable, aligned, NONEXECUTABLE linked data, and
    # retain its exact four bytes, registers, opcode, and access position.
    # Never normalize arithmetic immediates, MMIO, model data or code loads.
    functions = proof['proof']['audit']['functions'](directory)
    result = []
    for name, original, calls, literals in rows:
        words = original[:]
        # Must-value analysis of integer LUI definitions across actual
        # direct branches. Different values at a join become UNKNOWN;
        # equal values retain ALL reaching definition sites. Calls kill
        # caller-saved registers; FP arithmetic never kills integer aN.
        states = [None] * len(original)
        states[0] = (None,) * 32
        pending = [0]
        call_sites = {offset // 4 for offset, _ in calls}
        while pending:
            at = pending.pop()
            opword = original[at]
            op, dest = opword & 127, (opword >> 7) & 31
            state = list(states[at])
            integer_fp = op == 0x53 and (opword >> 25) in (0x50, 0x51, 0x60, 0x61, 0x70, 0x71)
            if op == 0x37 and dest:
                state[dest] = (opword & 0xfffff000, frozenset((at,)))
            elif dest and (op in (3, 0x13, 0x17, 0x33, 0x67, 0x6f) or integer_fp):
                state[dest] = None
            if at in call_sites:
                for reg in (1, 5, 6, 7, *range(10, 18), *range(28, 32)):
                    state[reg] = None
                following = [at + 1] if dest == 1 else []
            elif op == 0x63:
                immediate = ((opword >> 31) << 12) | (((opword >> 7) & 1) << 11) \
                    | (((opword >> 25) & 63) << 5) | (((opword >> 8) & 15) << 1)
                delta = proof['proof']['audit']['sign'](immediate, 13)
                assert not delta & 3
                following = [at + 1, at + delta // 4]
            elif op == 0x6f:
                assert not dest
                immediate = ((opword >> 31) << 20) | (((opword >> 21) & 1023) << 1) \
                    | (((opword >> 20) & 1) << 11) | (((opword >> 12) & 255) << 12)
                delta = proof['proof']['audit']['sign'](immediate, 21)
                assert not delta & 3
                following = [at + delta // 4]
            elif op == 0x67:
                following = []  # inherited decoder already admits only actual ABI returns/direct calls
            else:
                following = [at + 1]
            state = tuple(state)
            for target in following:
                assert 0 <= target <= len(original), (name, at, target)
                if target == len(original):
                    continue
                previous = states[target]
                if previous is None:
                    merged = state
                else:
                    merged = tuple((a[0], a[1] | b[1]) if a is not None and b is not None and a[0] == b[0]
                                   else None for a, b in zip(previous, state))
                if merged != previous:
                    states[target] = merged
                    pending.append(target)
        scalars = []
        for index, word in enumerate(original):
            opcode, rs1 = word & 127, (word >> 15) & 31
            if states[index] is None:
                continue
            definition = states[index][rs1]
            if opcode in (3, 7) and (word >> 12) & 7 == 2 and definition is not None:
                value, definitions = definition
                immediate = proof['proof']['audit']['sign'](word >> 20, 12)
                address = (value + immediate) & 0xffffffff
                section = ('text', 0x40000, 0x60000) if 0x40000 <= address < 0x60000 else ('conv', 0x64000, 0x6c000)
                label, start, end = section
                if start <= address and address + 4 <= end and not address & 3 \
                        and not any(at <= address < at + size for at, (size, _) in functions.items()):
                    data = (directory / (label + '.bin')).read_bytes()
                    raw = data[address-start:address-start+4]
                    assert len(raw) == 4
                    scalars.append((tuple(sorted(definitions)), index, raw))
                    for first in definitions:
                        words[first] &= 0xfff
                    words[index] &= 0xfffff
        result.append((name, words, calls, literals, scalars))
    return result


count = 0
# Removing the only eight-counter callers lets GCC specialize this one
# remaining serialized Packet body. It is NOT byte-identical. Check its
# complete direct-call multiset separately; original-path execution is an
# additional required gate, not supplied by this static audit.
changed_packet = 'MixedRecurrentCohort::Packet('
before_packet, after_packet = bodies(old, changed_packet), bodies(new, changed_packet)
assert len(before_packet) == len(after_packet) == 1
before_calls = Counter(callee for _, callee in before_packet[0][2])
after_calls = Counter(callee for _, callee in after_packet[0][2])
assert before_calls == after_calls
assert after_calls['PioneerQ38TidPacketQuiet()'] == 2
assert after_calls['q38_layer_return_recurrent_packet_seal'] == 3
assert after_calls['q38_layer_return_recurrent_seal'] == 2
names = (*(name for name in proof['proof']['names'] if name != changed_packet),
         'PioneerQ38TidPacketQuiet(', 'PioneerQ38TidFullQuiet(',
         'PioneerQ38TidInitiatorsQuiet(',
         'q38_layer_return_recurrent_packet_seal', 'q38_layer_return_recurrent_seal',
         'PioneerQ38RmsSum::Add(', 'PioneerQ38F32HiddenRms(',
         'PioneerQ38F32HiddenResidualRms(', 'PioneerQ38ProjectionQuantize(',
         'DensePartitionRoot::PioneerQ38SigmoidPrecision(',
         'DensePartitionRoot::PioneerQ38SigmoidDetail::ExpReduced(',
         'DensePartitionRoot::PioneerQ38SigmoidDetail::Divide(')
for name in names:
    before, after = bodies(old, name), bodies(new, name)
    left, right = scalar_literals(old, before), scalar_literals(new, after)
    if left != right:
        for a, b in zip(left, right):
            print('BODY_DIFFERENCE', a[0], b[0], 'words', len(a[1]), len(b[1]),
                  'calls', a[2], b[2], 'objectsEqual', a[3] == b[3], 'scalarsEqual', a[4] == b[4],
                  'wordChanges', [(i, hex(x), hex(y)) for i, (x, y) in enumerate(zip(a[1], b[1])) if x != y][:20], flush=True)
    assert left == right, ('instruction/call/exact-literal change beyond LINK/address relocation', name)
    count += len(before)
def direct_named_calls(directory, prefix):
    table = proof['proof']['audit']['functions'](directory)
    result, unresolved = [], []
    for start, (size, name) in table.items():
        if not name.startswith(prefix):
            continue
        base, section = (0x40000, 'text') if start < 0x60000 else (0x64000, 'conv')
        raw = (directory / (section + '.bin')).read_bytes()[start-base:start-base+size]
        words = struct.unpack('<' + 'I' * (size // 4), raw)
        for index, word in enumerate(words):
            opcode, rd, rs1 = word & 127, (word >> 7) & 31, (word >> 15) & 31
            pc = start + index * 4
            if opcode == 0x6f:
                immediate = ((word >> 31) << 20) | (((word >> 21) & 1023) << 1) \
                    | (((word >> 20) & 1) << 11) | (((word >> 12) & 255) << 12)
                address = (pc + proof['proof']['audit']['sign'](immediate, 21)) & 0xffffffff
                if not rd and start <= address < start + size:
                    continue
            elif opcode == 0x67:
                immediate = proof['proof']['audit']['sign'](word >> 20, 12)
                if not rd and rs1 == 1 and not immediate:
                    continue
                prior = words[index-1] if index else 0
                if prior & 127 != 0x17 or (prior >> 7) & 31 != rs1:
                    unresolved.append((name, pc-start, word))
                    continue
                address = (pc-4 + proof['proof']['audit']['sign'](prior & 0xfffff000, 32) + immediate) & 0xfffffffe
            else:
                continue
            assert rd in (0, 1) and address in table, (name, hex(pc), hex(address))
            result.append(table[address][1])
    # Snapshot already contains a compiler-generated indexed switch, not
    # a direct call. The complete image is pinned; do NOT claim that this
    # direct-call check proves switch targets or branch feasibility.
    assert len(unresolved) == int(prefix == 'DensePartitionRoot::Snapshot('), (prefix, unresolved)
    if unresolved:
        assert unresolved[0][2] == 0x00068067, unresolved  # actual jr a3, not an accepted arbitrary edge
        print('EXPLICITLY_UNRESOLVED_INDEXED_SWITCH', unresolved, flush=True)
    return result


for name, target, expected in (
    ('MixedRecurrentCohort::CounterSeal(', 'PioneerQ38CounterGather::Read(', 1),
    ('DensePartitionRoot::GatherCounters(', 'PioneerQ38CounterGather::Read(', 1),
    # The retained Snapshot is from the unchanged DENSE component. The
    # new flag is recurrent-only; do not count an unlinked source hook as
    # a real additional queued caller or claim dense-attention speedup.
    ('DensePartitionRoot::Snapshot(', 'DensePartitionRoot::GatherCounters(', 0),
    ('DensePartitionRoot::Admit(', 'DensePartitionRoot::GatherCounters(', 1),
):
    calls = direct_named_calls(new, name)
    assert sum(callee.startswith(target) for callee in calls) == expected, (name, calls)
print(f'PASS emitted bc01->a2d5/{count} selected arithmetic,transfer,release,commit,feedback,live-source+FIFO/TID guards '
      'byte-exact except authenticated LINK/literal relocation;actual8-read helper called by both coordinators;'
      'changed Admit/CounterSeal/gather and compiler-specialized Cohort::Packet bodies NOT included in equality assertion;'
      'specialized Packet retains exact direct-call multiset incl2packet-quiet+3source/packet+2full guards,NOT byte/path equivalence;'
      'Snapshot and Projection::Run indexed switches NOT covered by direct-call checks;'
      'STATIC ONLY,no model,native,device,Work or TPS proof')
