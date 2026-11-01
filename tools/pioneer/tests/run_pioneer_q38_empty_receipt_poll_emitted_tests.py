#!/usr/bin/env python3
"""Selected emitted bodies/calls only;no cycle or whole-model proof."""
import ast
from collections import Counter
import hashlib
import os
import pathlib
import runpy
import struct
import subprocess
import sys

assert len(sys.argv) == 2 and not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
new = pathlib.Path(sys.argv[1])
assert new.is_absolute() and not new.is_symlink()
old = new / 'unchanged-read-source'
here = pathlib.Path(__file__).resolve().parent
saved = sys.argv[:]
try:
    sys.argv = [str(here / 'run_pioneer_q38_read_source_boundary_emitted_tests.py'), str(old)]
    audit = runpy.run_path(sys.argv[0], run_name='__main__')
finally:
    sys.argv = saved
tree = ast.parse((here.parent / 'build_qwen38_empty_receipt_poll_root.py').read_text())
pins = next(ast.literal_eval(n.value) for n in tree.body if isinstance(n, ast.Assign)
            and any(isinstance(t, ast.Name) and t.id == 'PINS' for t in n.targets))
assert pins
for name, size, digest in pins:
    data = (new / name).read_bytes()
    assert len(data) == size and hashlib.sha256(data).hexdigest() == digest, name
bodies, literals = audit['bodies'], audit['literals']
names = tuple(name for name in audit['names'] if name != 'Projection::Run(') + (
    'q38_layer_return_recurrent_packet_seal', 'q38_layer_return_recurrent_direction_packet_seal')
count = 0
for name in names:
    before, after = literals(old, bodies(old, name)), literals(new, bodies(new, name))
    assert before == after, ('instructions,calls or exact readonly literals changed beyond LINK relocation', name)
    count += len(before)


def callback_body(directory, name):
    # The inherited CFG decoder deliberately refuses register callbacks.
    # Do NOT relax it. Separately compare complete callback-body bytes,
    # normalizing ONLY actual direct LINK relocations to named functions
    # and straight-line materializations of authenticated readonly objects.
    # Indirect instructions retain their exact words/offsets;their runtime
    # targets are explicitly unresolved,not manufactured from a register.
    tool = '/Volumes/HaikuBuildLocal/generated.riscv64/cross-tools-riscv64/bin/riscv64-unknown-haiku-objdump'
    table = subprocess.check_output([tool, '-tC', str(directory / 'program.elf')], text=True)
    functions = {}
    for line in table.splitlines():
        row = line.split(None, 5)
        # nm's lowercase t ALSO labels readonly OBJECTS in.text. Use the
        # actual ELF FUNCTION type,not that section-letter approximation.
        if len(row) == 6 and row[2] == 'F' and int(row[4], 16):
            functions.setdefault(int(row[0], 16), []).append((int(row[4], 16), row[5]))
        elif row and row[-1] == 'scalar_fault_park':
            assert int(row[0], 16) == 0x60000
            functions.setdefault(0x60000, []).append((0, 'scalar_fault_park'))
    matches = [(at, size) for at, rows in functions.items() for size, label in rows if label == name]
    assert len(matches) == 1, name
    start, size = matches[0]
    assert not (start | size) & 3 and 0x40000 <= start < start+size <= 0x60000
    raw = (directory / 'text.bin').read_bytes()[start-0x40000:start-0x40000+size]
    assert len(raw) == size
    words = list(struct.unpack('<'+'I'*(size//4), raw))
    original = words[:]
    direct, indirect = [], []
    sign = lambda value, bits: value-(1 << bits) if value & (1 << (bits-1)) else value
    constants = audit['audit']['proof']['proof']['objects'](directory)
    literals = []
    block_starts = {0}
    for i, word in enumerate(original):
        opcode = word & 127
        if opcode in (0x63, 0x6f, 0x67):
            block_starts.add(i+1)
        if opcode == 0x63:
            immediate = ((word >> 31) << 12) | (((word >> 7) & 1) << 11) \
                | (((word >> 25) & 63) << 5) | (((word >> 8) & 15) << 1)
            delta = sign(immediate, 13)
        elif opcode == 0x6f:
            immediate = ((word >> 31) << 20) | (((word >> 21) & 1023) << 1) \
                | (((word >> 20) & 1) << 11) | (((word >> 12) & 255) << 12)
            delta = sign(immediate, 21)
        else:
            continue
        assert not delta & 3
        block_starts.add(i+delta//4)
    known = {}
    for i, word in enumerate(original):
        if i in block_starts:
            known.clear()
        opcode = word & 127
        rd, rs1 = (word >> 7) & 31, (word >> 15) & 31
        assert opcode not in (7, 0x53), 'These callback bodies are integer-only'
        if opcode == 0x37 and rd:
            known[rd] = (i, word & 0xfffff000)
            continue
        # Track LUI definitions ONLY within one straight-line basic block.
        # Branch targets,fallthrough joins,calls and integer clobbers kill
        # them. Normalize ONLY real readonly OBJECT identity+all bytes.
        if opcode == 0x13 and not (word >> 12) & 7 and rd == rs1 and rd in known:
            first, high = known[rd]
            target = (high+sign(word >> 20, 12)) & 0xffffffff
            found = {(label, length, data, target-at) for at, length, label, data in constants
                     if at <= target < at+length}
            if found:
                assert len(found) == 1 and not any(at <= target < at+length for at, rows in functions.items()
                                                 for length, _ in rows), (name, hex(target))
                literals.append((first, i, next(iter(found))))
                words[first] &= 0xfff
                words[i] &= 0xfffff
        if rd and opcode in (3, 0x13, 0x17, 0x33, 0x67, 0x6f):
            known.pop(rd, None)
        if opcode in (0x63, 0x67, 0x6f):
            known.clear()
    for i, word in enumerate(original):
        if word & 127 != 0x67:
            continue
        rd, rs1, low = (word >> 7) & 31, (word >> 15) & 31, sign(word >> 20, 12)
        assert rs1 and rd in (0, 1)
        prior = original[i-1] if i else 0
        if prior & 127 == 0x17 and (prior >> 7) & 31 == rs1:
            target = (start+(i-1)*4+sign(prior & 0xfffff000, 32)+low) & 0xfffffffe
            assert target in functions, (name, hex(target))
            direct.append((i, tuple(sorted(label for _, label in functions[target]))))
            words[i-1] &= 0xfff
            words[i] &= 0xfffff
        elif not (rd == 0 and rs1 == 1 and low == 0):
            indirect.append((i, word))
    assert indirect, 'This separate comparison is ONLY for explicit unresolved callback bodies'
    return words, direct, indirect, literals


for name in ('q38_pushed_receipt_shared', 'q38_pushed_begin_shared', 'q38_pushed_end_shared'):
    before, after = callback_body(old, name), callback_body(new, name)
    if before != after:
        print('CALLBACK_DIFFERENCE', name, 'words', len(before[0]), len(after[0]),
              'wordChanges', [(i, hex(a), hex(b)) for i, (a, b) in enumerate(zip(before[0], after[0])) if a != b][:16],
              'directEqual', before[1] == after[1], 'indirectEqual', before[2] == after[2],
              'literalEqual', before[3] == after[3], flush=True)
    assert before == after, ('callback body changes beyond exact direct named LINK relocation', name)
    print('EXPLICIT_UNRESOLVED_CALLBACKS_BYTE_PRESERVED', name, before[2], flush=True)
for prefix in ('DensePartitionRoot::Transfer(', 'MixedRecurrentCohort::Packet('):
    calls = audit['audit']['direct_named_calls']
    assert Counter(calls(old, prefix)) == Counter(calls(new, prefix)), prefix
wrapper = bodies(new, 'q38_layer_return_recurrent_empty_receipt_seal')
assert len(wrapper) == 1
name, words, sites, _ = wrapper[0]
assert [callee for _, callee in sites] == ['q38_layer_return_recurrent_direction_packet_seal']
assert words.count(0x00000593) == 1, 'exact li a1,0 in empty-only wrapper'
# PushedWait is inlined into Projection::Run. Its indexed switch is NOT
# decoded as a direct-body/path proof. Preserve this explicit omission;
# genuine original-byte empty-branch execution is a separate required gate.
symbol_text = subprocess.check_output([
    '/Volumes/HaikuBuildLocal/generated.riscv64/cross-tools-riscv64/bin/riscv64-unknown-haiku-nm',
    '-nC', str(new / 'program.elf')], text=True)
assert ' Projection::Run(' in symbol_text  # presence ONLY;do not run the refusing CFG decoder here
print(f'PASS emitted 0f67->3718/{count} selected arithmetic,release,commit,feedback,source/FIFO/TID/gather+'
      'bodies byte-exact except authenticated LINK/readonly-literal relocation;'
      '3 complete receipt/begin/end callback bodies separately byte-equal except direct named LINK+'
      'authenticated readonly OBJECT address relocations,'
      'indirect instructions unchanged but callback targets NOT statically proved;'
      'unchanged transfer/Packet direct-call multisets;new empty-only wrapper always selects false;'
      'inlined PushedWait/Projection::Run argument+indexed-switch paths NOT proved by static equivalence;'
      'STATIC ONLY,no model,native,physical,Work or TPS proof')
