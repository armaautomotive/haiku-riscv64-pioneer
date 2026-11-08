#!/usr/bin/env python3
"""Compare a separately captured P100A hidden vector with pinned CPU data.

This is an offline diagnostic, not a quality acceptance or Work-mode check.
"""
import array
import hashlib
import math
import os
from pathlib import Path
import sys

from compare_pioneer_q38_observed_trace import CPU_PINS, require, stable_bytes
from verify_pioneer_q38_qgw8_cpu_hidden_snapshot import (
    SNAPSHOT_BYTES as CPU_BYTES, TRACE_BYTES, WIDTH, inspect,
)

CARD_BYTES = (16 + WIDTH + 1) * 4
CARD_TAG = 0x2026110700010129
CPU_SNAPSHOT_PIN = 'ce1ed4778227d6deb4bc0fd85440facf2a680c7d60b2e7d92c60e74abd73c98a'


def words32(data, kind):
    words = array.array(kind)
    require(words.itemsize == 4, '32-bit word host required')
    words.frombytes(data)
    if sys.byteorder != 'little':
        words.byteswap()
    return words


def decode_card(data, expected_hash):
    require(len(expected_hash) == 64 and all(c in '0123456789abcdef' for c in expected_hash), 'expected SHA-256')
    require(len(data) == CARD_BYTES and hashlib.sha256(data).hexdigest() == expected_hash, 'card snapshot length/hash')
    words = words32(data, 'I')
    require(list(words[:10]) == [0x31444851, 1, 0, 248319, 64, WIDTH, 1, 1,
                                CARD_TAG & 0xffffffff, CARD_TAG >> 32], 'card snapshot identity')
    require(bool(words[10] or words[11]) and list(words[12:16]) == [2, 0, 0, 0], 'card generation/chunks/reserved')
    require(words[-1] == 0x454e4448, 'card snapshot footer')
    values = words32(data[64:-4], 'f')
    require(all(math.isfinite(x) for x in values), 'nonfinite card hidden data')
    return values, (words[11] << 32) | words[10]


def metrics(reference, actual):
    require(len(reference) == len(actual) == WIDTH, 'hidden vector width')
    square_error = math.fsum((float(b) - float(a)) ** 2 for a, b in zip(reference, actual))
    reference_norm = math.fsum(float(a) ** 2 for a in reference)
    actual_norm = math.fsum(float(b) ** 2 for b in actual)
    dot = math.fsum(float(a) * float(b) for a, b in zip(reference, actual))
    cosine = dot / math.sqrt(reference_norm * actual_norm) if reference_norm and actual_norm else None
    return math.sqrt(square_error / (reference_norm + 1e-300)), max(abs(float(b) - float(a)) for a, b in zip(reference, actual)), cosine


def main():
    require(len(sys.argv) == 5, 'usage: script ABS_PINNED_CPU_TRACE ABS_CPU_HIDDEN ABS_CARD_HIDDEN CARD_SHA256')
    require(not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ), 'unexpected role overrides')
    trace, cpu, card = map(Path, sys.argv[1:4])
    require(all(p.is_absolute() for p in (trace, cpu, card)), 'absolute files required')
    trace_data = stable_bytes(trace, TRACE_BYTES)
    cpu_data = stable_bytes(cpu, CPU_BYTES)
    require(hashlib.sha256(cpu_data).hexdigest() == CPU_SNAPSHOT_PIN, 'converted CPU snapshot hash')
    inspect(trace_data, cpu_data, CPU_PINS[0])
    card_data = stable_bytes(card, CARD_BYTES)
    actual, generation = decode_card(card_data, sys.argv[4])
    cpu_values = words32(cpu_data[64:-4], 'f')
    for name, reference in (('original', cpu_values[:WIDTH]), ('converted', cpu_values[WIDTH:])):
        relative, maximum, cosine = metrics(reference, actual)
        print(f'QGW8_HIDDEN_COMPARE reference={name} relative_l2={relative:.12g} '
              f'max_abs={maximum:.12g} cosine={cosine if cosine is not None else "undefined"}')
    print(f'QGW8_HIDDEN_DATA sha256={sys.argv[4]} generation={generation}; '
          'DIAGNOSTIC_ONLY_NO_QUALITY_HEAD_OR_SPEED_ACCEPTANCE')


if __name__ == '__main__':
    try:
        main()
    except (ValueError, OSError) as error:
        print(f'ERROR: {error}', file=sys.stderr)
        sys.exit(1)
