#!/usr/bin/env python3
"""Cross-check raw CPU-only layer DATA against an earlier pinned final hidden vector."""
import argparse
import array
import hashlib
import math
import os
import pathlib
import sys
from compare_pioneer_q38_observed_trace import CPU_PINS, require, stable_bytes
from compare_pioneer_q38_qgw8_layer_trace import TRACE_BYTES, WORDS
from compare_pioneer_q38_qgw8_hidden import CPU_SNAPSHOT_PIN
from verify_pioneer_q38_qgw8_cpu_hidden_snapshot import SNAPSHOT_BYTES, TRACE_BYTES as CPU_TRACE_BYTES, inspect

def verify(raw, trace, snapshot):
    require(len(raw) == TRACE_BYTES, 'converted layer length')
    require(hashlib.sha256(snapshot).hexdigest() == CPU_SNAPSHOT_PIN, 'prior CPU snapshot pin')
    inspect(trace, snapshot, CPU_PINS[0])
    values = array.array('f'); require(values.itemsize == 4, '32-bit float host')
    values.frombytes(raw)
    if sys.byteorder != 'little':
        values.byteswap()
    require(all(math.isfinite(v) for v in values), 'nonfinite converted layer')
    require(raw[-WORDS*4:] == snapshot[64+WORDS*4:64+2*WORDS*4], 'converted final layer differs from prior pinned CPU snapshot')
    return hashlib.sha256(raw).hexdigest()

def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('layers', type=pathlib.Path)
    parser.add_argument('original_cpu_trace', type=pathlib.Path)
    parser.add_argument('prior_cpu_snapshot', type=pathlib.Path)
    args = parser.parse_args(argv)
    paths = (args.layers, args.original_cpu_trace, args.prior_cpu_snapshot)
    require(all(p.is_absolute() and not p.is_symlink() for p in paths), 'absolute nonsymlink files required')
    require(not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ), 'role override')
    raw = stable_bytes(args.layers, TRACE_BYTES)
    trace = stable_bytes(args.original_cpu_trace, CPU_TRACE_BYTES)
    snapshot = stable_bytes(args.prior_cpu_snapshot, SNAPSHOT_BYTES)
    digest = verify(raw, trace, snapshot)
    print(f'CONVERTED_CPU_LAYER_SHA256={digest} final_layer_matches_prior_pinned_CPU_snapshot=1')
    print('CPU DATA ONLY: intermediate layers and origin are not independently authenticated; NO DEVICE OR SPEED')
    return 0

if __name__ == '__main__':
    try:
        sys.exit(main())
    except (ValueError, OSError) as error:
        print('CONVERTED_CPU_LAYER_REFUSAL ' + str(error), file=sys.stderr)
        sys.exit(2)
