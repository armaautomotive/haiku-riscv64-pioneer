#!/usr/bin/env python3
"""Synthetic checks for the CPU-only snapshot verifier."""
import array
import hashlib
import sys
from verify_pioneer_q38_qgw8_cpu_hidden_snapshot import HIDDEN_WORDS, VOCAB_WORDS, WIDTH, inspect

assert sys.byteorder == 'little' and array.array('I').itemsize == 4
header = array.array('I', [0x31524651, 1, 0, 248319, 0, 1, 0, 0, 65, WIDTH, VOCAB_WORDS, 0, 0, 0, 0, 0])
trace = header + array.array('I', [0x3f800000]) * HIDDEN_WORDS + array.array('I', [0]) * VOCAB_WORDS + array.array('I', [0x454e4451])
raw = trace.tobytes()
pin = hashlib.sha256(raw).hexdigest()
snapshot = array.array('I', [0x38484351, 1, 248319, 64, WIDTH, 2, 0, 0] + [0] * 8)
snapshot += array.array('I', [0x3f800000]) * WIDTH + array.array('I', [0x40000000]) * WIDTH + array.array('I', [0x454e4448])
result = inspect(raw, snapshot.tobytes(), pin)
assert result['bit_differences'] == WIDTH and result['relative_l2'] == 1 and result['max_abs'] == 1
for index in (0, 2, 6, 8, 16, 16 + WIDTH, len(snapshot) - 1):
    bad = snapshot[:]
    bad[index] = 0x7f800000 if index == 16 + WIDTH else bad[index] ^ 1
    try:
        inspect(raw, bad.tobytes(), pin)
    except ValueError:
        pass
    else:
        raise AssertionError('malformed snapshot accepted at ' + str(index))
try:
    inspect(raw, snapshot.tobytes(), '0' * 64)
except ValueError:
    pass
else:
    raise AssertionError('wrong original trace pin accepted')
print('PASS CPU-only hidden verifier pin/shape/finiteness/identity/delta refusals; no device or model')
