#!/usr/bin/env python3
"""Bound the WHOLE 9f -> queued-E image difference; NOT execution authority.

This is deliberately specific to one experimental image, not a general
permission to ignore changed sections. Three named new Q4 function bodies
are bound by size/hash, not proved correct here. Every other byte must be
unchanged, zero alignment, the explicit 40-byte field-index table, or one
of nine symbol-resolved relocations to a COMPLETE unchanged object/helper.
Queue/backpressure/failure, history, native and hardware tests remain due.
"""
import ast
import hashlib
import pathlib
import re
import struct
import subprocess
import tempfile

from pioneer_q38_vector_q4_relocations import BASES

WORK = '_ZL21PioneerQ38ExactQ4WorkPVKj'
BFP = '_ZN25PioneerQ38NativeBfpMatvec5SetupI28PioneerQ38ExactQ4WorkEmitterEEbRT_j'
PRIMARY = '_ZN24PioneerQ38ExactQ4Primary5SetupI28PioneerQ38ExactQ4WorkEmitterEEbRT_'
OLD_DOT = '_ZN30PioneerQ38ExactQ4TwoProductDot3RunIN23PioneerQ38ExactQ4Affine12PanelEmitterI28PioneerQ38ExactQ4WorkEmitterEEEEbRT_'
WINDOW = '_ZN26PioneerQ38PreparedQ4Layout11GroupWindowEPVKjjS1_RNS_6WindowE'
COMMON = {
    '_ZL7Counterj', '_ZL7IdleAllv', '_ZL5Routej',
    '_ZL14PreparedQ4ReadjjjjRjS_', '_ZL13SourceMatchesRA16_Kj',
    '_ZL16PreparedQ4Inputsj', '_ZL15PreparedQ4GroupPVKjjRjS1_', WINDOW,
}
SUBJECTS = {
    WORK: (0x6ac90, 5624, '9920f11460b17d6e4c9ea0a5233843a5d234f0bdbfadf9a106b10d2ca02a2b18'),
    PRIMARY: (0x6c470, 616, 'c79146a895ae7ba8470ad91985f09a41cc96e006ba972a3713efb618b4be7cbe'),
    BFP: (0x6c6d8, 2280, '64ff2bc8bde0c87e318f1ad7ebb3871c9d0cfe5f7eded921e0950c9e51b0249f'),
}
# section, LUI/AUIPC address, ADDI/JALR address, COMPLETE target identity.
# All unchanged instructions, including between each pair, remain audited.
RELOCATIONS = (
    ('service', 0x6a6a4, 0x6a6b4, '_ZZL7CounterjE6offset', 'OBJECT'),
    ('service', 0x6a794, 0x6a798, '_ZZL5RoutejE7columns', 'OBJECT'),
    ('service', 0x6abc4, 0x6abc8, WINDOW, 'FUNC'),
    ('fetch', 0x701d8, 0x701e0, 'CSWTCH', 'OBJECT'),
    ('fetch', 0x70424, 0x70428, 'CSWTCH', 'OBJECT'),
    ('affine', 0x70b5c, 0x70b6c, '_ZZL5SetupvE6values', 'OBJECT'),
    ('affine', 0x70b60, 0x70b74, '_ZZL5SetupvE7indices', 'OBJECT'),
    ('affine', 0x71754, 0x7175c, 'CSWTCH', 'OBJECT'),
    ('text', 0x72ac8, 0x72ad0, 'CSWTCH', 'OBJECT'),
)


def require(condition, message):
    if not condition:
        raise ValueError(message)


def canonical(name):
    if name in ('_ZN27PioneerQ38MixedWorkerLayoutL4PeerEj', '_ZN30PioneerQ38DensePartitionLayoutL4PeerEj'):
        return 'EXACT_SHARED_MIXED_DENSE_PEER'
    return re.sub(r'^CSWTCH\.\d+$', 'CSWTCH', name)


def load_image(directory, tool):
    """Use actual ELF symbols and check every raw ROM against its ELF section."""
    directory = pathlib.Path(directory)
    require(directory.is_absolute() and directory.is_dir() and not directory.is_symlink(), 'real image directory')
    elf = directory / 'program.elf'
    require(elf.is_file() and not elf.is_symlink(), 'real ELF')
    raw = {}
    rows = subprocess.check_output([tool+'objdump', '-h', str(elf)], text=True).splitlines()
    allocated = {}
    for i, row in enumerate(rows):
        m = re.fullmatch(r'\s*\d+\s+(\S+)\s+([0-9a-fA-F]+)\s+([0-9a-fA-F]+)\s+.*', row)
        if m and 'ALLOC' in rows[i+1]:
            name = m[1].removeprefix('.')
            require(name in BASES and name not in allocated and int(m[3], 16) == BASES[name], 'exact allocated section map')
            allocated[name] = int(m[2], 16)
    require(set(allocated) == set(BASES), 'all and only worker ROM sections')
    with tempfile.TemporaryDirectory(prefix='pioneer-queued-rom-audit-', dir='/private/tmp') as name:
        for section in BASES:
            source = directory / (section+'.bin')
            require(source.is_file() and not source.is_symlink(), 'real ROM '+section)
            data = source.read_bytes()
            require(len(data) == allocated[section] and len(data) % 4 == 0, 'whole ROM size '+section)
            extracted = pathlib.Path(name) / section
            subprocess.run([tool+'objcopy', '-O', 'binary', '-j', '.'+section, str(elf), str(extracted)], check=True)
            require(data == extracted.read_bytes(), 'raw ROM differs from actual ELF '+section)
            raw[section] = data
    symbols = {}
    for row in subprocess.check_output([tool+'readelf', '-sW', str(elf)], text=True).splitlines():
        p = row.split()
        if len(p) == 8 and p[0].endswith(':') and p[3] in ('FUNC', 'OBJECT') and int(p[2]):
            at, size, kind = int(p[1], 16), int(p[2]), p[3]
            item = (canonical(p[7]), size, kind)
            require(at not in symbols or symbols[at] == item, 'conflicting symbol aliases')
            symbols[at] = item
    return raw, symbols


def audit_data(old, new, old_symbols, new_symbols):
    """Pure mutation-testable whole-image audit, independent of image hashes."""
    raws = old, new
    maps = old_symbols, new_symbols
    require(set(old) == set(new) == set(BASES), 'exact whole ROM inventory')
    require(len(old['service']) == 17512 and len(new['service']) == 20416, 'bounded whole service sizes')
    require(len(old['text']) == 7844 and len(new['text']) == 7884, 'bounded whole main sizes')
    for section in ('boot', 'globalworker', 'precise', 'dense', 'ffn', 'gdn'):
        require(old[section] == new[section], 'whole unchanged '+section)
    require(old['service'][:9888] == new['service'][:9888], 'whole original convolution prefix')
    require(new['text'][0x1db8:0x1de0] == struct.pack('<10I', 1, 50, 64, 65, 72, 73, 76, 84, 86, 124), 'whole exact new field-index table')
    require(old['text'][0x1db8:] == new['text'][0x1de0:], 'whole old 236-byte readonly tail moved +40')
    require(new_symbols.get(0x73db8) == ('CSWTCH', 40, 'OBJECT'), 'new index table symbol')

    def contents(side, at, size):
        for section, base in BASES.items():
            if base <= at and at+size <= base+len(raws[side][section]):
                return raws[side][section][at-base:at-base+size]
        raise ValueError('unmapped whole symbol '+hex(at))

    # Inventory/partition BOTH tails; no unnamed bytes, extra functions, or
    # executable padding are silently waived by binding the three subjects.
    for side, expected in enumerate((COMMON | {WORK, BFP, OLD_DOT}, COMMON | set(SUBJECTS))):
        limit = 0x68000 + len(raws[side]['service'])
        tail = sorted((at, *item) for at, item in maps[side].items() if 0x6a6a0 <= at < limit)
        require(len(tail) == len(expected) and {r[1] for r in tail} == expected, 'explicit complete service tail')
        cursor = 0x6a6a0
        for at, name, size, kind in tail:
            require(kind == 'FUNC' and size > 0 and cursor <= at and at-cursor <= 12 and at+size <= limit, 'tail extent '+name)
            require(contents(side, cursor, at-cursor) == bytes(at-cursor), 'zero tail alignment')
            cursor = at+size
        require(cursor == limit, 'no trailing unaudited service bytes')
    for name, (at, size, digest) in SUBJECTS.items():
        require(new_symbols.get(at) == (name, size, 'FUNC'), 'exact new subject symbol '+name)
        require(hashlib.sha256(contents(1, at, size)).hexdigest() == digest, 'whole bound new subject '+name)

    # Normalize ONLY the nine authenticated address immediates. All opcode,
    # register, branch, arithmetic and other bytes must still compare exact.
    normalized = {s:bytearray(data) for s, data in new.items()}
    targets = []
    for section, hi_at, lo_at, name, kind in RELOCATIONS:
        base = BASES[section]
        words = []
        for side in (0, 1):
            a = struct.unpack_from('<I', raws[side][section], hi_at-base)[0]
            b = struct.unpack_from('<I', raws[side][section], lo_at-base)[0]
            words.append((a, b))
        require(words[0][0] & 0xfff == words[1][0] & 0xfff and words[0][1] & 0xfffff == words[1][1] & 0xfffff, 'unchanged relocation opcode/registers')
        addresses = []
        for a, b in words:
            lo = (b >> 20) - (4096 if b >> 31 else 0)
            if kind == 'FUNC':
                require(lo_at == hi_at+4 and a & 0xfff == 0x097 and b & 0xfffff == 0x080e7, 'exact AUIPC/JALR call pair')
                hi = (a & 0xfffff000) - (2**32 if a >> 31 else 0)
                at = (hi_at+hi+lo) & 0xffffffff
            else:
                require(a & 0x7f == 0x37 and b & 0x707f == 0x13 and (b >> 15) & 31 == (a >> 7) & 31, 'exact LUI/ADDI data pair')
                at = ((a & 0xfffff000)+lo) & 0xffffffff
            addresses.append(at)
        items = [maps[side].get(at) for side, at in enumerate(addresses)]
        require(items[0] is not None and items[0] == items[1] and items[0][0] == name and items[0][2] == kind, 'complete target symbol identity '+name)
        size = items[0][1]
        require(contents(0, addresses[0], size) == contents(1, addresses[1], size), 'complete unchanged target body '+name)
        require(addresses[1]-addresses[0] == (1128 if kind == 'FUNC' else 40), 'exact bounded target relocation')
        for at in (hi_at, lo_at):
            normalized[section][at-base:at-base+4] = old[section][at-base:at-base+4]
        targets.append((section, hex(lo_at), name, size))

    for section in ('fetch', 'affine'):
        require(old[section] == normalized[section], 'whole relocated '+section)
    require(old['text'][:0x1db8] == normalized['text'][:0x1db8], 'whole unchanged main instruction prefix')
    for name in COMMON:
        records = [[(at, size) for at, (n, size, kind) in m.items() if n == name and kind == 'FUNC'] for m in maps]
        require(all(len(r) == 1 for r in records) and records[0][0][1] == records[1][0][1], 'whole common function identity')
        (a, size), (b, _) = records[0][0], records[1][0]
        require(contents(0, a, size) == bytes(normalized['service'][b-0x68000:b-0x68000+size]), 'whole unchanged common helper '+name)
    return targets


def audit(control, image, tool):
    control, image = pathlib.Path(control), pathlib.Path(image)
    here = pathlib.Path(__file__).resolve().parent.parent
    nodes = ast.parse((here/'build_qwen38_instruction_prefetch_worker.py').read_text()).body
    pins = next(ast.literal_eval(n.value) for n in nodes if isinstance(n, ast.Assign) and any(isinstance(t, ast.Name) and t.id == 'PINS' for t in n.targets))
    for name, size, digest in pins:
        data = (control/name).read_bytes()
        require(len(data) == size and hashlib.sha256(data).hexdigest() == digest, 'authenticated accepted 9f parent '+name)
    old, old_symbols = load_image(control, tool)
    new, new_symbols = load_image(image, tool)
    targets = audit_data(old, new, old_symbols, new_symbols)
    for target in targets:
        print('AUDITED_QUEUED_Q4_WHOLE_TARGET', *target, flush=True)
    print('PASS_QUEUED_Q4_WHOLE_ROM_DIFFERENCE sixWholeSectionsExact originalService9888Exact commonService8WholeBodiesExact nineResolvedRelocations oldReadonlyTail236Exact newIndexTable40Exact newSubjects3HashBound;NOT_QUEUE_FAILURE_HISTORY_NATIVE_HARDWARE_OR_SPEED_QUALIFICATION', flush=True)
    return targets
