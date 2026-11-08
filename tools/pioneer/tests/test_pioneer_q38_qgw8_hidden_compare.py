#!/usr/bin/env python3
"""Synthetic parser and arithmetic checks; does not run hardware."""
import hashlib
import math
import struct

from compare_pioneer_q38_qgw8_hidden import CARD_TAG, WIDTH, decode_card, metrics


def fixture():
    header = [0x31444851, 1, 0, 248319, 64, WIDTH, 1, 1,
              CARD_TAG & 0xffffffff, CARD_TAG >> 32, 5, 0, 2, 0, 0, 0]
    return bytearray(struct.pack('<16I', *header) + struct.pack('<%df' % WIDTH, *([1.0] * WIDTH))
                     + struct.pack('<I', 0x454e4448))


def rejected(data, digest=None):
    try:
        decode_card(data, digest or hashlib.sha256(data).hexdigest())
    except ValueError:
        return
    raise AssertionError('invalid snapshot accepted')


data = fixture()
digest = hashlib.sha256(data).hexdigest()
actual, generation = decode_card(data, digest)
assert generation == 5 and len(actual) == WIDTH
relative, maximum, cosine = metrics(actual, [2.0] * WIDTH)
assert abs(relative - 1) < 1e-12 and maximum == 1 and abs(cosine - 1) < 1e-12
assert metrics([0.0] * WIDTH, [0.0] * WIDTH)[2] is None
rejected(data, '0' * 64)
for offset, value in [(0, 0), (3, 3), (8, 0), (10, 0), (12, 3), (13, 1)]:
    broken = fixture()
    struct.pack_into('<I', broken, offset * 4, value)
    rejected(broken)
broken = fixture()
struct.pack_into('<I', broken, 64, 0x7fc00000)
rejected(broken)
broken = fixture()
broken[-1] ^= 1
rejected(broken)
rejected(data[:-1])
print('PASS tag129 snapshot identity, hash, finite data, and arithmetic; synthetic only')
