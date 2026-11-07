#!/usr/bin/env python3
"""Check matched-band card transcripts and report scoped cycles/error only.

Transcript comparison is not proof of process exit, driver restoration,
repeatability, full-model correctness, or model throughput. The caller must
separately establish the actual parent exit and independent postflight.
"""
import hashlib
import math
from pathlib import Path
import re
import struct
import sys

if len(sys.argv) != 6:
    raise SystemExit('use BFP4_BAND Q4_INPUT Q4_EXPECTED Q4_PHYSICAL_LOG MATCHED_PARENT_LOG')

def checked(path, size, digest):
    data = Path(path).read_bytes()
    if len(data) != size or hashlib.sha256(data).hexdigest() != digest:
        raise ValueError('pinned input mismatch')
    return data

band = checked(sys.argv[1], 102544, '916ef0df61974947e4eff96294bb4e306ec8aa1400f2db22699bd3a0e6482eb0')
inputs = checked(sys.argv[2], 10240, '5dbd04b0cd25a054d499db2bde60e55c5ecf345491350e9147016cf6d4527f99')
q4 = checked(sys.argv[3], 128, '7e7d73b158e0032a15678e63963c481d73ffe3fa911982b23a1bce0c8fdb722b')
qlog = checked(sys.argv[4], 15920, 'eb908182a31e3a63929b1d825443c03b519cb7d888dbe0cdcb1bdd1164c15712')
blog = Path(sys.argv[5]).read_bytes()
if struct.unpack_from('<4I', band) != (0x34504252, 1, 5120, 0) or band[92176:102416] != inputs:
    raise ValueError('sample/input identity mismatch')
qwords = struct.unpack('<32I', q4)
bwords = struct.unpack_from('<32I', band, 102416)
qpat = re.compile(rb'^NATIVE_COMPACT_Q4_STREAM_RETURNED_WORD sample0 tensor7 first0 width5120 pass([01]) row([0-9]+) got([0-9a-f]{8}) expected([0-9a-f]{8})$')
bpat = re.compile(rb'^NATIVE_GDDR_BAND_RETURNED_WORD sample0 first0 pass([01]) row([0-9]+) got([0-9a-f]{8}) expected([0-9a-f]{8})$')
def words(log, pattern, expected):
    seen = set()
    for line in log.splitlines():
        m = pattern.fullmatch(line)
        if not m:
            continue
        pair = (int(m[1]), int(m[2]))
        if pair in seen or pair[1] >= 32:
            raise ValueError('duplicate or out-of-range card output')
        if int(m[3], 16) != expected[pair[1]] or int(m[4], 16) != expected[pair[1]]:
            raise ValueError('card output mismatch')
        seen.add(pair)
    if seen != {(p, row) for p in range(2) for row in range(32)}:
        raise ValueError('incomplete card output')
words(qlog, qpat, qwords)
words(blog, bpat, bwords)
bt = re.findall(rb'^BFP4_GDDR_BAND_CYCLES sample0 pass([01]) cycles([0-9]+);INCLUDES_GDDR_AND_COMPUTE_NOT_MODEL_SPEED$', blog, re.M)
if len(bt) != 2 or [int(x[0]) for x in bt] != [0, 1] or any(int(x[1]) == 0 for x in bt):
    raise ValueError('incomplete BFP4 timing')
if b'MATCHED_BFP4_GDDR_KNOWN_CLEAN_NO_MODEL_SPEED_CLAIM\n' not in blog:
    raise ValueError('missing parent completion marker')
stable = b'P100A_RELOAD driver55 tag2026102100010012 generation20 quiesced0 cold0 active_owner0 open_count1 guard1;CPU_METADATA_ONLY'
if stable not in blog.splitlines():
    raise ValueError('missing restored-driver metadata')
qt = re.findall(rb'^NATIVE_COMPACT_Q4_TIMING sample0 width5120 pass([01]) state1 start[0-9]+ end[0-9]+ cycles([0-9]+);CARD_CYCLES_ONLY_NO_FREQUENCY_ASSUMPTION$', qlog, re.M)
if len(qt) != 2 or [int(x[0]) for x in qt] != [0, 1] or any(int(x[1]) == 0 for x in qt):
    raise ValueError('incomplete Q4 timing')
qcycles = tuple(int(x[1]) for x in qt)
bcycles = tuple(int(x[1]) for x in bt)
a = struct.unpack('<32f', q4)
b = struct.unpack_from('<32f', band, 102416)
if not all(math.isfinite(v) for v in (*a, *b)):
    raise ValueError('nonfinite output')
energy = sum(x*x for x in a)
if energy <= 0:
    raise ValueError('zero Q4 output')
relative = math.sqrt(sum((x-y)**2 for x, y in zip(a, b)) / energy)
print(f'MATCHED_CARD_SAMPLE0 q4_cycles{qcycles[0]},{qcycles[1]} '
      f'bfp4_cycles{bcycles[0]},{bcycles[1]} mean_ratio{sum(qcycles)/sum(bcycles):.6f} '
      f'output_relative_l2{relative:.9f}; '
      'TWO_PASSES_ONE_BAND_TRANSCRIPTS_ONLY_ACTUAL_EXIT_AND_POSTFLIGHT_REQUIRED_NO_MODEL_TPS')
