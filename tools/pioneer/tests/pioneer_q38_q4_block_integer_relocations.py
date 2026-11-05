#!/usr/bin/env python3
"""Whole9f->block-integer boundary;new Work body BOUND,not statically proved."""
import ast
import functools
import hashlib
from pathlib import Path
import struct
import subprocess
from pioneer_q38_queued_q4_block_relocations import load_image as load_base_image, require, WORK, BFP, OLD_DOT, WINDOW
from pioneer_q38_vector_q4_relocations import BASES

# Assembly supplies no STT_FUNC extent for this handler. Bind its WHOLE
# instruction block, including both stop-loop jumps, rather than accepting
# arbitrary equal unmapped call addresses. This is not invented FUNC metadata.
FAULT_AT = 0x20080
FAULT_RECORD = ('block_fault_park', 36, 'BOUND_ASM_BLOCK')
FAULT_SHA = '6c0d6875fc0932e93ad5d98c1b0b9f0396f6ee6dfb3f90d72c1be471c55f30a9'


def load_image(directory, tool):
    raw, symbols = load_base_image(directory, tool)
    rows = [row.split() for row in subprocess.check_output(
        [tool + 'readelf', '-sW', str(Path(directory) / 'program.elf')], text=True).splitlines()]
    for name, address in (('block_fault_park', FAULT_AT), ('block_fault_stopped', 0x2009c)):
        records = [row[1:7] for row in rows if len(row) == 8 and row[7] == name]
        require(records == [[f'{address:08x}', '0', 'NOTYPE', 'GLOBAL', 'DEFAULT', '2']],
                'exact actual assembly fault label ' + name)
    require(FAULT_AT not in symbols and symbols.get(FAULT_AT + 36) ==
            ('EXACT_SHARED_MIXED_DENSE_PEER', 72, 'FUNC'), 'complete explicit assembly fault boundary')
    symbols[FAULT_AT] = FAULT_RECORD
    return raw, symbols

COMMON = {
 '_ZL7Counterj': ((0x6a6a0,56),(0x6a6a0,56)),
 '_ZL7IdleAllv': ((0x6a6d8,140),(0x6a6d8,140)),
 '_ZL5Routej': ((0x6a764,120),(0x6a764,120)),
 '_ZL14PreparedQ4ReadjjjjRjS_': ((0x6a7dc,164),(0x6a7dc,164)),
 '_ZL13SourceMatchesRA16_Kj': ((0x6a880,136),(0x6a880,136)),
 '_ZL16PreparedQ4Inputsj': ((0x6a908,516),(0x6a908,516)),
 '_ZL15PreparedQ4GroupPVKjjRjS1_': ((0x6ab0c,388),(0x6ab0c,388)),
 OLD_DOT: ((0x6bb6c,692),(0x6c4bc,692)),
 WINDOW: ((0x6be20,488),(0x6c770,488)),
 BFP: ((0x6c008,1120),(0x6c958,1120)),
}
SUBJECTS = (
 (0x6ac90,3804,'14332b751e76154bcb4d3f25df2afb00b16c599d6d5ee7aa439dfcfb3d4bde71'),
 (0x6ac90,6188,'2ac82c0f0edc9690b9379123323efa70385f9a83f4d4822bb9da974f28f7f429'),
)


@functools.lru_cache(maxsize=1)
def control_records():
    path = Path(__file__).resolve().parent.parent / 'build_qwen38_instruction_prefetch_worker.py'
    return next(ast.literal_eval(node.value) for node in ast.parse(path.read_text()).body
                if isinstance(node, ast.Assign) and any(isinstance(t, ast.Name) and t.id == 'PINS' for t in node.targets))


def audit_data(old, new, old_symbols, new_symbols):
    require(set(old) == set(new) == set(BASES), 'whole ROM inventory')
    for name, size, digest in control_records():
        if name == 'program.elf':
            continue
        data = old[name.removesuffix('.bin')]
        require(len(data) == size and hashlib.sha256(data).hexdigest() == digest, 'whole authenticated9f ' + name)
    require(len(new['service']) == 19896, 'exact bounded new service size')
    for section in BASES:
        if section != 'service':
            require(old[section] == new[section], 'whole unchanged ' + section)
    require(old['service'][:9888] == new['service'][:9888], 'complete convolution prefix')
    raws, maps = (old, new), (old_symbols, new_symbols)

    def contents(side, at, size):
        for section, base in BASES.items():
            if base <= at and at + size <= base + len(raws[side][section]):
                return raws[side][section][at - base:at - base + size]
        raise ValueError('unmapped complete function ' + hex(at))

    for side in (0, 1):
        require(maps[side].get(FAULT_AT) == FAULT_RECORD, 'exact bound assembly fault record')
        require(hashlib.sha256(contents(side, FAULT_AT, 36)).hexdigest() == FAULT_SHA,
                'whole assembly fault handler and both stop-loop jumps')

    for side in (0, 1):
        expected = {name: locations[side] for name, locations in COMMON.items()}
        at, size, digest = SUBJECTS[side]
        expected[WORK] = (at, size)
        limit = 0x68000 + len(raws[side]['service'])
        tail = sorted((at, *item) for at, item in maps[side].items() if 0x6a6a0 <= at < limit)
        require(len(tail) == len(expected), 'no extra or missing tail symbols')
        require({name: (at, size) for at, name, size, kind in tail} == expected
                and all(kind == 'FUNC' for _, _, _, kind in tail), 'exact complete tail functions')
        cursor = 0x6a6a0
        for at, name, size, kind in tail:
            require(cursor <= at and at - cursor <= 12 and size > 0 and at + size <= limit, 'nonoverlapping whole function ' + name)
            require(contents(side, cursor, at - cursor) == bytes(at - cursor), 'zero alignment only')
            cursor = at + size
        require(cursor == limit, 'no unexamined service suffix')
        at, size, digest = SUBJECTS[side]
        require(hashlib.sha256(contents(side, at, size)).hexdigest() == digest, 'whole bound Work body')

    targets = []
    for name, ((old_at, size), (new_at, new_size)) in COMMON.items():
        require(size == new_size, 'complete helper extent ' + name)
        before, after = contents(0, old_at, size), contents(1, new_at, size)
        normalized = bytearray(after)
        different = {i for i in range(0, size, 4) if before[i:i + 4] != after[i:i + 4]}
        while different:
            offset = min(different)
            if struct.unpack_from('<I', before, offset)[0] & 127 == 0x67:
                offset -= 4
            require(0 <= offset <= size - 8, 'complete call-pair extent')
            pairs = [struct.unpack_from('<II', body, offset) for body in (before, after)]
            require(pairs[0][0] & 0xfff == pairs[1][0] & 0xfff
                    and pairs[0][1] & 0xfffff == pairs[1][1] & 0xfffff, 'unchanged call opcode/register bits')
            addresses = []
            for side, (hi, lo) in enumerate(pairs):
                require(hi & 127 == 0x17 and lo & 0x707f == 0x67
                        and (hi >> 7) & 31 == (lo >> 15) & 31
                        and (lo >> 7) & 31 in (0, 1), 'actual AUIPC/JALR or tail-call pair')
                high = (hi & 0xfffff000) - (2**32 if hi >> 31 else 0)
                low = (lo >> 20) - (4096 if lo >> 31 else 0)
                target = ((old_at if side == 0 else new_at) + offset + high + low) & 0xffffffff
                require(not target & 3, 'aligned complete target')
                addresses.append(target)
            a, b = (maps[side].get(addresses[side]) for side in (0, 1))
            require(a is not None and a == b and (a[2] == 'FUNC' or
                    (a == FAULT_RECORD and addresses == [FAULT_AT, FAULT_AT])),
                    f'same complete actual target identity {name}+{offset:x} {addresses!r} {a!r} {b!r}')
            require(contents(0, addresses[0], a[1]) == contents(1, addresses[1], a[1]), 'whole unchanged relocation target')
            normalized[offset:offset + 8] = before[offset:offset + 8]
            different.difference_update((offset, offset + 4))
            targets.append((name, offset, a[0], a[1], addresses[0], addresses[1]))
        require(before == bytes(normalized), 'ENTIRE helper exact after explicit calls ' + name)
    return targets


def audit(control, image, tool):
    old, old_symbols = load_image(control, tool)
    new, new_symbols = load_image(image, tool)
    targets = audit_data(old, new, old_symbols, new_symbols)
    for name, offset, target, size, before, after in targets:
        print('BOUND_BLOCK_INTEGER_CALL', name, hex(offset), target, size, hex(before), hex(after), flush=True)
    print('BOUND_BLOCK_INTEGER_WHOLE_IMAGE nineWholeSections_9888Prefix_tenWholeHelpers_resolvedCompleteCalls_oneBoundNewWorkBody;NOT_ARITHMETIC_OWNERSHIP_NATIVE_HARDWARE_OR_SPEED_PROOF', flush=True)
    return targets
