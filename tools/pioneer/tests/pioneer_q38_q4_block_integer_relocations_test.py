#!/usr/bin/env python3
"""Mutation checks for complete code boundary,not hardware/quality proof."""
import copy
import sys
from pioneer_q38_q4_block_integer_relocations import audit, audit_data, load_image, FAULT_AT

assert __debug__ and len(sys.argv) == 3
tool = '/Volumes/HaikuBuildLocal/generated.riscv64/cross-tools-riscv64/bin/riscv64-unknown-haiku-'
audit(sys.argv[1], sys.argv[2], tool)
old, old_symbols = load_image(sys.argv[1], tool)
new, new_symbols = load_image(sys.argv[2], tool)
refused = 0


def reject(a, b, c, d):
    global refused
    try:
        audit_data(a, b, c, d)
    except ValueError:
        refused += 1
        return
    raise AssertionError('mutation escaped whole-image audit')


for i in range(len(new['service'])):
    mutated = dict(new)
    data = bytearray(new['service'])
    data[i] ^= 1
    mutated['service'] = bytes(data)
    reject(old, mutated, old_symbols, new_symbols)
for section, data in new.items():
    if section == 'service':
        continue
    for i in {0, len(data) // 2, len(data) - 1}:
        changed = bytearray(data)
        changed[i] ^= 1
        mutated = dict(new)
        mutated[section] = bytes(changed)
        reject(old, mutated, old_symbols, new_symbols)
        other = dict(old)
        other[section] = mutated[section]
        reject(other, mutated, old_symbols, new_symbols)
symbol_refusals = 0
for side, symbols in enumerate((old_symbols, new_symbols)):
    limit = 0x68000 + len((old, new)[side]['service'])
    for at, (name, size, kind) in symbols.items():
        if not (0x6a6a0 <= at < limit or at == FAULT_AT):
            continue
        for mode in range(5):
            changed = copy.copy(symbols)
            if mode == 0:
                del changed[at]
            elif mode == 1:
                changed[at] = (name + 'X', size, kind)
            elif mode == 2:
                changed[at] = (name, size + 4, kind)
            elif mode == 3:
                changed[at] = (name, size, 'OBJECT')
            else:
                changed[at + 1] = changed.pop(at)
            reject(old, new, changed if not side else old_symbols, changed if side else new_symbols)
            symbol_refusals += 1
print(f'PASS_BLOCK_INTEGER_IMAGE_BOUNDARY everyNewServiceByte{len(new["service"])} otherSectionMutations{refused-len(new["service"])-symbol_refusals} symbolMutations{symbol_refusals};NO_NATIVE_MODEL_OR_TPS')
