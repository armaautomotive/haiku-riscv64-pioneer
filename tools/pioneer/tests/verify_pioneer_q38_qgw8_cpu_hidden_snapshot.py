#!/usr/bin/env python3
"""Compare CPU-only QGW8 final hidden DATA to the pinned original trace."""
import array
import hashlib
import math
import os
from pathlib import Path
import sys
from compare_pioneer_q38_observed_trace import CPU_PINS, HIDDEN_WORDS, VOCAB_WORDS, decode_trace, require, stable_bytes

WIDTH = 5120
SNAPSHOT_BYTES = (16 + 2 * WIDTH + 1) * 4
TRACE_BYTES = (16 + HIDDEN_WORDS + VOCAB_WORDS + 1) * 4

def inspect(trace_data, snapshot_data, expected_pin):
    require(hashlib.sha256(trace_data).hexdigest() == expected_pin, 'original trace hash')
    original = decode_trace(trace_data, 0, False)
    require(len(snapshot_data) == SNAPSHOT_BYTES, 'snapshot length')
    words = array.array('I')
    require(words.itemsize == 4, '32-bit words required')
    words.frombytes(snapshot_data)
    if sys.byteorder != 'little':
        words.byteswap()
    require(list(words[:6]) == [0x38484351, 1, 248319, 64, WIDTH, 2], 'snapshot family/input/dimensions')
    require(words[6] == original['header'][4] and words[7] < VOCAB_WORDS and not any(words[8:16]), 'snapshot top IDs/reserved')
    require(words[-1] == 0x454e4448, 'snapshot footer')
    baseline = words[16:16 + WIDTH]
    candidate = words[16 + WIDTH:16 + 2 * WIDTH]
    require(baseline == original['bits'][64 * WIDTH:65 * WIDTH], 'snapshot original hidden differs from pinned trace')
    floats = array.array('f')
    require(floats.itemsize == 4, 'binary32 host required')
    floats.frombytes(snapshot_data[64:-4])
    if sys.byteorder != 'little':
        floats.byteswap()
    require(all(math.isfinite(x) for x in floats), 'nonfinite hidden data')
    reference, actual = floats[:WIDTH], floats[WIDTH:]
    squared = math.fsum((float(b) - float(a)) ** 2 for a, b in zip(reference, actual))
    norm = math.fsum(float(a) ** 2 for a in reference)
    maximum = max(abs(float(b) - float(a)) for a, b in zip(reference, actual))
    relative = math.sqrt(squared / (norm + 1e-300))
    return {'snapshot_sha256': hashlib.sha256(snapshot_data).hexdigest(), 'original_top': words[6],
            'converted_top': words[7], 'bit_differences': sum(a != b for a, b in zip(baseline, candidate)),
            'relative_l2': relative, 'max_abs': maximum}

def main():
    require(len(sys.argv) == 3, 'usage: script ABS_PINNED_CPU_TOKEN0 ABS_CPU_HIDDEN_SNAPSHOT')
    require(not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ), 'unexpected role overrides')
    trace, snapshot = map(Path, sys.argv[1:])
    require(trace.is_absolute() and snapshot.is_absolute(), 'absolute files required')
    result = inspect(stable_bytes(trace, TRACE_BYTES), stable_bytes(snapshot, SNAPSHOT_BYTES), CPU_PINS[0])
    print('CPU_HIDDEN_SNAPSHOT verified_original_pin original_top={original_top} converted_top={converted_top} '
          'bit_differences={bit_differences} relative_l2={relative_l2:.9g} max_abs={max_abs:.9g} '
          'sha256={snapshot_sha256}; CPU_ONLY_NOT_DEVICE'.format(**result))
    print('No numerical acceptance, output quality, P100A execution or speed is implied.')

if __name__ == '__main__':
    main()
