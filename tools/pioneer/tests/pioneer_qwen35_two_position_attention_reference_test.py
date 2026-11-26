#!/usr/bin/env python3
"""Exercise the independent two-position attention reference."""
import math
import struct
from pioneer_qwen35_two_position_attention_reference import compare, two_position_float_context

query = [0.0] * 4096
key = [0.0] * 1024
value = [float(head + 2) for head in range(4) for _ in range(256)]
gate = [0.5] * 4096
prior = [0.0] * 1024 + [float(head) for head in range(4) for _ in range(256)]
actual = two_position_float_context(query, key, value, gate, prior)
decoded = struct.unpack('<4096f', actual)
for head in range(16):
    assert all(x == (head // 4 + 1) * 0.5 for x in decoded[head * 256:(head + 1) * 256])
assert compare(actual, actual)['mismatches'] == 0
changed = bytearray(actual)
struct.pack_into('<f', changed, 4095 * 4, 100.0)
assert compare(changed, actual)['mismatches'] == 1
for bad_query, bad_gate in ((query[:-1], gate), ([math.nan] + query[1:], gate), (query, [2.0] + gate[1:])):
    try:
        two_position_float_context(bad_query, key, value, bad_gate, prior)
        raise AssertionError('invalid attention input accepted')
    except ValueError:
        pass
print('PASS independent two-position attention formula and invalid input checks; NO HARDWARE')
