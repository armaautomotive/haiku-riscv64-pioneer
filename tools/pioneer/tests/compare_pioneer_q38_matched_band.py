#!/usr/bin/env python3
"""Compare one private matched BFP4 prediction against the archived Q4 card result.

This reads private model-derived data and prints aggregate metrics only. It
does not open the GPU, run BFP4 on silicon, or establish model quality/speed.
"""
import hashlib
import math
from pathlib import Path
import re
import struct
import sys

if len(sys.argv) != 5:
    raise SystemExit('use BFP4_BAND Q4_INPUT Q4_EXPECTED Q4_PHYSICAL_LOG')

def checked(path, size, digest):
    data = Path(path).read_bytes()
    if len(data) != size or hashlib.sha256(data).hexdigest() != digest:
        raise ValueError('input size or pinned SHA-256 differs')
    return data

band = checked(sys.argv[1], 102544, '916ef0df61974947e4eff96294bb4e306ec8aa1400f2db22699bd3a0e6482eb0')
inputs = checked(sys.argv[2], 10240, '5dbd04b0cd25a054d499db2bde60e55c5ecf345491350e9147016cf6d4527f99')
q4 = checked(sys.argv[3], 128, '7e7d73b158e0032a15678e63963c481d73ffe3fa911982b23a1bce0c8fdb722b')
log = checked(sys.argv[4], 15920, 'eb908182a31e3a63929b1d825443c03b519cb7d888dbe0cdcb1bdd1164c15712')
if struct.unpack_from('<4I', band) != (0x34504252, 1, 5120, 0) or band[92176:102416] != inputs:
    raise ValueError('band identity or matched input differs')
pattern = re.compile(rb'^NATIVE_COMPACT_Q4_STREAM_RETURNED_WORD sample0 tensor7 first0 width5120 pass([01]) row([0-9]+) got([0-9a-f]{8}) expected([0-9a-f]{8})$')
seen = set()
for line in log.splitlines():
    match = pattern.fullmatch(line)
    if not match:
        continue
    index = (int(match[1]), int(match[2]))
    if index in seen or index[1] >= 32:
        raise ValueError('duplicate or invalid card row')
    expected = struct.unpack_from('<I', q4, index[1] * 4)[0]
    if int(match[3], 16) != expected or int(match[4], 16) != expected:
        raise ValueError('archived card output differs from Q4 prediction')
    seen.add(index)
if seen != {(p, row) for p in range(2) for row in range(32)}:
    raise ValueError('archived Q4 card output is incomplete')
a = struct.unpack('<32f', q4)
b = struct.unpack_from('<32f', band, 102416)
if not all(math.isfinite(v) for v in (*a, *b)):
    raise ValueError('nonfinite output')
energy = sum(x*x for x in a)
if energy <= 0:
    raise ValueError('zero original output')
relative = math.sqrt(sum((x-y)**2 for x, y in zip(a, b)) / energy)
maximum = max(abs(x-y) for x, y in zip(a, b))
print('MATCHED_SAMPLE0 Q4_CARD_WORDS64_EXACT BFP4_CPU_PREDICTION32 input_bytes10240 '
      f'relative_l2{relative:.9f} max_absolute{maximum:.9f}; '
      'NO_NEW_DEVICE_RUN_NO_BFP4_SILICON_NO_MODEL_QUALITY_OR_SPEED')
