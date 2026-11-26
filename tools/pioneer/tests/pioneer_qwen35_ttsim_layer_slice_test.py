#!/usr/bin/env python3
"""Independently audit a simulator-only isolated-layer fixture."""
from pathlib import Path
import os
import struct
import sys

if len(sys.argv) not in (5, 6) or (len(sys.argv) == 6 and sys.argv[5] != '--warm-state'):
    raise SystemExit('usage: test full-fixture slice-fixture layer original-start-step [--warm-state]')
warm = len(sys.argv) == 6
source, sliced = map(Path, sys.argv[1:3])
layer, start = map(int, sys.argv[3:5])
assert 0 <= layer < 32 and start >= 1
parse = lambda p: [line.split() for line in p.read_text().splitlines()]
old = parse(source / 'layout.txt'); new = parse(sliced / 'layout.txt')
old_config = [tuple(map(int, x[1:])) for x in old if x[0] == 'C']
new_config = [tuple(map(int, x[1:])) for x in new if x[0] == 'C']
assert len(old_config) == len(new_config) == 1 and old_config[0][3] == 1
count = old_config[0][2]
assert count in (500, 524, 532)
assert new_config[0][0] == layer + 1 and new_config[0][3] == 0
executed, records = new_config[0][1:3]
assert 1 <= executed < count and records == executed + 1 and start + executed <= count
assert (sliced / 'layer-slice.marker').is_file()
assert (sliced / 'warm-layer.marker').is_file() == warm
context_only = (sliced / 'attention-context.marker').is_file()
projection_markers = [name for name in ('query', 'key', 'value')
                      if (sliced / f'{name}-projection.marker').is_file()]
assert len(projection_markers) <= 1
projection = projection_markers[0] if projection_markers else None
stateless = (sliced / 'warm-stateless.marker').is_file()
assert stateless == bool(warm and projection)
for name in ('weights.bin', 'templates.bin', 'command.bin'):
    assert os.path.samefile(source / name, sliced / name)
for kind in ('W', 'S'):
    assert [x for x in old if x[0] == kind] == [x for x in new if x[0] == kind]
layouts = {}
for kind in ('I', 'O'):
    parts = [tuple(map(int, x[1:])) for x in new if x[0] == kind]
    next_row = 0; banks = set()
    for bank, address, first, rows in parts:
        assert 0 <= bank < 7 and bank not in banks and first == next_row
        assert address % 4096 == 0 and rows > 0 and rows % 16 == 0
        banks.add(bank); next_row += rows
    output_rows = 8192 if projection == 'query' else 1024 if projection else 4096 if context_only else 9216
    assert next_row == (output_rows if kind == 'O' else 9216)
    layouts[kind] = parts

def row_space(record, word_offset):
    offset = 64 + word_offset * 4
    rows, row_bytes, fragments, unit = struct.unpack_from('<4I', record, offset)
    assert 0 < fragments <= 7
    parts = [struct.unpack_from('<4I', record, offset + 16 + 16 * i) for i in range(fragments)]
    return rows, row_bytes, unit, parts


def residual_space(record, mode, core):
    header = struct.unpack_from('<16I', record)
    assert header[4] == core and header[5] == 0x31535235
    magic, version, actual_core, actual_mode = struct.unpack_from('<4I', record, 64)
    assert (magic, version, actual_core, actual_mode) == (0x31535235, 1, core, mode)
    rows, row_bytes, unit, parts = row_space(record, 20)
    assert (rows, row_bytes, unit) == (9216, 4, 16)
    return parts

zero_expected = set();warm_expected = set()
with (source / 'records.bin').open('rb') as original, (sliced / 'records.bin').open('rb') as replay:
    for core in range(120):
        original.seek((core * count + start) * 2048)
        for step in range(executed):
            before = bytearray(original.read(2048)); after = replay.read(2048)
            assert len(before) == len(after) == 2048
            assert struct.unpack_from('<I', before, 12)[0] == start + step + 1
            struct.pack_into('<I', before, 12, step + 1)
            assert before == after
            if step == 0:
                assert residual_space(after, 1, core) == layouts['I']
            if step + 1 == executed:
                if projection:
                    header = struct.unpack_from('<16I', after)
                    magic, version, actual_core, _, _, result_rows = struct.unpack_from('<6I', after, 64)
                    expected_rows = 8192 if projection == 'query' else 1024
                    assert (header[5], magic, actual_core, result_rows) == (0x31504635, 0x31504635, core, expected_rows)
                    rows, row_bytes, unit, parts = row_space(after, 110)
                    assert rows >= expected_rows and (row_bytes, unit) == (4, 16)
                    cropped = []
                    for bank, address, first, count_rows in parts:
                        if first >= expected_rows:
                            break
                        cropped.append((bank, address, first, min(count_rows, expected_rows - first)))
                    assert cropped == layouts['O']
                elif context_only:
                    header = struct.unpack_from('<16I', after)
                    magic, version, actual_core, phase = struct.unpack_from('<4I', after, 64)
                    assert (header[5], magic, version, actual_core, phase) == (0x31484635, 0x31484635, 1, core, 2)
                    rows, row_bytes, unit, parts = row_space(after, 86)
                    assert (rows, row_bytes, unit) == (9216, 4, 16)
                    cropped = []
                    for bank, address, first, count_rows in parts:
                        if first >= 4096:
                            break
                        cropped.append((bank, address, first, min(count_rows, 4096 - first)))
                    assert cropped == layouts['O']
                else:
                    assert residual_space(after, 2, core) == layouts['O']
            magic = struct.unpack_from('<I', after, 20)[0]
            if magic in (0x31435635, 0x31534435) or (warm and magic == 0x31414435):
                if magic == 0x31435635:
                    actual_layer, position = struct.unpack_from('<2I', after, 64 + 3 * 4)
                    rows, row_bytes, unit, parts = row_space(after, 85 if warm else 53)
                    assert (rows, row_bytes, unit) == (8192, 12, 1)
                elif magic == 0x31534435:
                    actual_layer, position = struct.unpack_from('<2I', after, 64 + 7 * 4)
                    rows, row_bytes, unit, parts = row_space(after, 183 if warm else 151)
                    assert (rows, row_bytes, unit) == (32, 65536, 1)
                else:
                    actual_layer, position = struct.unpack_from('<2I', after, 64 + 5 * 4)
                    rows, row_bytes, unit, parts = row_space(after, 119)
                    assert (rows, row_bytes, unit) == (8, 32 * 512, 1)
                assert actual_layer == layer and position == 0
                ranges = {(bank, address, part_rows * row_bytes) for bank, address, _, part_rows in parts}
                if core == 0:
                    zero_expected.update(ranges)
                    if warm:
                        kind = 'H' if magic == 0x31435635 else 'D' if magic == 0x31534435 else 'K'
                        warm_expected.update((kind, layer, row_bytes, bank, address, first, part_rows)
                                             for bank, address, first, part_rows in parts)
                else:
                    assert ranges <= zero_expected
        assert replay.read(2048) == bytes(2048)
    assert not replay.read(1)
# The injected hidden vector cannot overwrite any staged weights or streams.
weights = [tuple(map(int, x[1:])) for x in new if x[0] == 'W']
streams = [tuple(map(int, x[1:])) for x in new if x[0] == 'S']
zeros = [tuple(map(int, x[1:])) for x in new if x[0] == 'Z']
assert len(zeros) == len(set(zeros)) and (set(zeros) == set() if warm else set(zeros) == zero_expected)
assert bool(zeros) == ((layer & 3) != 3 and not warm)
if warm and not stateless:
    rows = parse(sliced / 'warm-state-layout.txt')
    actual = {(x[0], *map(int, x[1:])) for x in rows}
    assert len(actual) == len(rows) and actual == warm_expected
elif stateless:
    assert not (sliced / 'warm-state-layout.txt').exists()
assert len(streams) == 120
for bank, address, _, rows in layouts['I']:
    end = address + rows * 4
    for wbank, waddress, size, _ in weights:
        assert bank != wbank or end <= waddress or waddress + size <= address
    for _, sbank, saddress in streams:
        assert bank != sbank or end <= saddress or saddress + count * 2048 <= address
protected = [(bank, address, size) for bank, address, size, _ in weights]
protected += [(bank, address, count * 2048) for _, bank, address in streams]
protected += [(bank, address, rows * 4) for kind in ('I', 'O') for bank, address, _, rows in layouts[kind]]
for index, (bank, address, size) in enumerate(zeros):
    assert 0 <= bank < 7 and size > 0 and size % 4 == 0 and address % 4096 == 0
    for other_bank, other_address, other_size in protected + zeros[:index]:
        assert bank != other_bank or address + size <= other_address or other_address + other_size <= address
print(f'PASS isolated layer {layer} replay records, input/output ranges and weight separation on 120 workers; NO HARDWARE')
