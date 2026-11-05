#!/usr/bin/env python3
"""Whole-image binding + exhaustive changed-region byte rejection; OFFBOARD."""
import copy
import pathlib
import sys

from pioneer_q38_queued_q4_block_relocations import (
    BASES, COMMON, RELOCATIONS, SUBJECTS, audit, audit_data, load_image, require,
)

require(__debug__ and len(sys.argv) == 3, 'accepted9f queuedE absolute image directories')
control, image = map(pathlib.Path, sys.argv[1:])
tool = '/Volumes/HaikuBuildLocal/generated.riscv64/cross-tools-riscv64/bin/riscv64-unknown-haiku-'
audit(control, image, tool)
old, old_symbols = load_image(control, tool)
new, new_symbols = load_image(image, tool)
require(len(audit_data(old, new, old_symbols, new_symbols)) == 9, 'one positive whole audit')

rejected_bytes = 0
for section in ('service', 'fetch', 'affine', 'text'):
    # Every byte of ALL four changed sections, including new subjects,
    # opcodes/registers/immediates, both table tails and alignment gaps.
    mutable = dict(new)
    for index in range(len(new[section])):
        altered = bytearray(new[section])
        altered[index] ^= 1
        mutable[section] = bytes(altered)
        try:
            audit_data(old, mutable, old_symbols, new_symbols)
        except ValueError:
            rejected_bytes += 1
        else:
            raise AssertionError(('accepted changed byte', section, index))
    print('PASS_QUEUED_WHOLE_SECTION_EVERY_BYTE_REFUSED', section, len(new[section]), flush=True)

other = 0
for section in set(BASES)-{'service', 'fetch', 'affine', 'text'}:
    for index in sorted({0, len(new[section])//2, len(new[section])-1}):
        altered = bytearray(new[section]); altered[index] ^= 1
        try:
            audit_data(old, {**new, section:bytes(altered)}, old_symbols, new_symbols)
        except ValueError:
            other += 1
        else:
            raise AssertionError(('accepted old-section corruption', section, index))

rejected_symbols = 0
addresses = {0x73db8}
for side_at, (name, _, _) in new_symbols.items():
    if 0x6a6a0 <= side_at < 0x74000 and (name in COMMON | set(SUBJECTS) or any(name == r[3] for r in RELOCATIONS)):
        addresses.add(side_at)
for at in sorted(addresses):
    name, size, kind = new_symbols[at]
    for bad in (None, (name+'UNEXPECTED', size, kind), (name, size+4, kind),
                (name, size, 'OBJECT' if kind == 'FUNC' else 'FUNC')):
        modified = copy.copy(new_symbols)
        if bad is None:
            del modified[at]
        else:
            modified[at] = bad
        try:
            audit_data(old, new, old_symbols, modified)
        except ValueError:
            rejected_symbols += 1
        else:
            raise AssertionError(('accepted symbol corruption', hex(at), bad))
require(rejected_bytes == sum(len(new[s]) for s in ('service', 'fetch', 'affine', 'text')), 'whole changed-section byte coverage')
print('PASS_QUEUED_Q4_RELOCATION_MUTATIONS changed_region_bytes'+str(rejected_bytes),
      'other_region_samples'+str(other), 'symbol_corruptions'+str(rejected_symbols),
      'NO_QUEUE_FAILURE_HISTORY_NATIVE_HARDWARE_OR_TPS_GRANT', flush=True)
