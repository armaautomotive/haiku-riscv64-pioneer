#!/usr/bin/env python3
"""Compare a completed dense-stage diagnostic DATA file; never access the card."""
import argparse
import array
import hashlib
import math
import os
from pathlib import Path
import re
import sys
from compare_pioneer_q38_observed_trace import difference, require, stable_bytes

WIDTH = 5120
TRACE_BYTES = 64 * WIDTH * 4
REFERENCE_HASH = 'cb6544d071ce526de7ec7cab66189b2ca39f73ecb33111dbd46b0bfeed2c5f8d'
BASELINE_HASH = '103312ce324384782d322b51a457d654c4c5c1fef181ceecc730aa6a3d82af2e'

def unpack(data):
    bits, values = array.array('I'), array.array('f')
    require(bits.itemsize == 4 and values.itemsize == 4, '32-bit words required')
    bits.frombytes(data); values.frombytes(data)
    if sys.byteorder != 'little':
        bits.byteswap(); values.byteswap()
    require(all(math.isfinite(v) for v in values), 'nonfinite data')
    return {'bits': bits, 'floats': values}

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('reference', type=Path)
    p.add_argument('baseline', type=Path)
    p.add_argument('candidate', type=Path)
    p.add_argument('--candidate-sha256', required=True)
    args = p.parse_args()
    require(not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ), 'role override')
    require(re.fullmatch(r'[0-9a-f]{64}', args.candidate_sha256) is not None, 'candidate hash')
    items = ((args.reference, 2*WIDTH*4, REFERENCE_HASH), (args.baseline, TRACE_BYTES, BASELINE_HASH),
             (args.candidate, TRACE_BYTES, args.candidate_sha256))
    snapshots = []
    for path, size, digest in items:
        require(path.is_absolute() and not path.is_symlink(), 'absolute nonsymlink inputs required')
        raw = stable_bytes(path, size)
        require(hashlib.sha256(raw).hexdigest() == digest, 'input hash mismatch')
        snapshots.append(unpack(raw))
    reference, baseline, candidate = snapshots
    for stage, name in enumerate(('projection', 'residual')):
        changed, maximum, relative = difference(reference, candidate, stage*WIDTH, WIDTH)
        require(math.isfinite(maximum) and math.isfinite(relative), 'invalid stage comparison')
        print(f'{name} bit_differences={changed} max_abs={maximum:.12g} relative_l2={relative:.12g}')
    for layer in range(2,64):
        changed, maximum, relative = difference(baseline, candidate, layer*WIDTH, WIDTH)
        print(f'layer={layer} changed={changed} max_abs={maximum:.12g} relative_l2={relative:.12g}')
    print('RAW DATA ONLY: hash does not prove source, completion, stage identity, correctness or speed')

if __name__ == '__main__':
    try:
        main()
    except (ValueError, OSError) as error:
        print('DENSE_STAGE_COMPARISON_REFUSED ' + str(error), file=sys.stderr)
        sys.exit(2)
