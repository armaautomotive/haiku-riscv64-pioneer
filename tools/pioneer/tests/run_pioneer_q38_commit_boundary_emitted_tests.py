#!/usr/bin/env python3
"""Actual RV32 call edges,not execution or timing permission."""
import hashlib
import os
import pathlib
import runpy
import struct
import sys

assert len(sys.argv) == 3 and not any(key.startswith(('PIONEER_', 'TT_PIONEER_')) for key in os.environ)
old, new = map(pathlib.Path, sys.argv[1:])
here = pathlib.Path(__file__).resolve().parent
saved = sys.argv[:]
try:
    # Reuse the existing emitted-RV32 decoder only AFTER its complete
    # old951/new48b2 authentication and Conv->GDN edge audit has passed.
    sys.argv = [str(here / 'run_pioneer_q38_retained_conv_emitted_tests.py'),
                str(old / 'unchanged-frontier'), str(old)]
    audit = runpy.run_path(sys.argv[0], run_name='__main__')
finally:
    sys.argv = saved
pins = audit['literal'](here.parent / 'build_qwen38_commit_boundary_root.py', 'PINS')
assert pins and new.is_absolute() and not new.is_symlink()
for name, size, digest in pins:
    data = (new / name).read_bytes()
    assert len(data) == size and hashlib.sha256(data).hexdigest() == digest
edges = audit['edges']
prefix = 'GlobalChain::'
before, after = edges(old, prefix + 'StateSeal('), edges(new, prefix + 'StateSeal(')
for check in ('InputSeal(', 'FeedbackSeal('):
    assert sum(target.startswith(prefix + check) for _, target in before) == 1
    assert not any(target.startswith(prefix + check) for _, target in after)
assert sum(target.startswith(prefix + 'HeadSeal(') for _, target in before) == 1
assert sum(target.startswith(prefix + 'HeadSeal(') for _, target in after) == 1
before, after = edges(old, prefix + 'CommitState('), edges(new, prefix + 'CommitState(')
for check in ('InputSeal(', 'FeedbackSeal('):
    assert sum(target.startswith(prefix + check) for _, target in before) == 1
    assert sum(target.startswith(prefix + check) for _, target in after) == 2
# GCC emits duplicated full/tail-chunk paths:4/7 sites,not the3/3 source
# expressions. Count decoded linked instructions,never assumed source sites.
for check, count in (('StateSeal(', 4), ('Packet(', 7), ('Transfer(', 1)):
    assert sum(target.startswith(prefix + check) for _, target in before) == count
    assert sum(target.startswith(prefix + check) for _, target in after) == count
for directory in (old, new):
    packet = edges(directory, prefix + 'Packet(')
    assert sum(target.startswith(prefix + 'HeadSeal(') for _, target in packet) == 2


def body(directory, needle, relocated=False):
    matches = [(start, size) for start, (size, name) in audit['functions'](directory).items()
               if name.startswith(needle)]
    assert len(matches) == 1
    start, size = matches[0]
    base = 0x40000 if start < 0x60000 else 0x64000
    data = (directory / ('text.bin' if base == 0x40000 else 'conv.bin')).read_bytes()
    assert size and base <= start < start + size <= base + len(data)
    raw = data[start-base:start-base+size]
    if not relocated:
        return raw
    words = list(struct.unpack('<' + 'I' * (size // 4), raw))
    targets = []
    # Keep every opcode,register,branch,load/store and data immediate.
    # Normalize ONLY an authenticated direct function call's LINK displacement
    # after decoding and retaining its exact destination-function name.
    for pc, target in edges(directory, needle):
        index = (pc - start) // 4
        if words[index] & 127 == 0x6f:
            words[index] &= 0xfff
        else:
            assert index and words[index] & 127 == 0x67 and words[index-1] & 127 == 0x17
            words[index-1] &= 0xfff
            words[index] &= 0xfffff
        targets.append((pc-start, target))
    return words, targets


assert body(old, prefix + 'Packet(') == body(new, prefix + 'Packet('), 'entire Packet raw bytes'
for name in ('Transfer(', 'HeadSeal(', 'InputSeal(', 'FeedbackSeal(', 'Scope('):
    assert body(old, prefix + name, True) == body(new, prefix + name, True), ('unchanged checked body/link-only fixups', name)
assert len(audit['unresolved_jumps']) == 2  # inherited StageConv switches ONLY
print('PASS actual old48b2/new83677 linked RV32 direct call sites/StateSeal numeric scans1->0 '
      'but its HeadSeal preserved/CommitState numeric entry+exit sites1->2/'
      'both retain4 StateSeal+7 Packet+1 Transfer emitted sites/both Packet still2 HeadSeal sites/'
      'ENTIRE Packet raw bytes identical/Transfer+HeadSeal+InputSeal+FeedbackSeal+Scope '
      'instructions+exact call targets identical except authenticated LINK-only displacements;'
      'STATIC only,NOT branch feasibility,transaction execution,original model,native,physical or TPS proof')
