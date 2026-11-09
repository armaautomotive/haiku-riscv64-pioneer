#!/usr/bin/env python3
"""Compare raw position-zero layer DATA to pinned CPU controls; no device IO."""
import argparse
import array
import hashlib
import math
import os
import pathlib
import re
import sys
from compare_pioneer_q38_observed_trace import difference, load_cpu, require, stable_bytes

LAYERS, WORDS = 64, 5120
TRACE_BYTES = LAYERS * WORDS * 4

def compare(cpu, data, threshold):
    require(len(data) == TRACE_BYTES, 'raw trace length')
    require(math.isfinite(threshold) and threshold >= 0, 'threshold must be finite and nonnegative')
    bits, values = array.array('I'), array.array('f')
    require(bits.itemsize == 4 and values.itemsize == 4, '32-bit host words required')
    bits.frombytes(data); values.frombytes(data)
    if sys.byteorder != 'little':
        bits.byteswap(); values.byteswap()
    require(all(math.isfinite(v) for v in values), 'nonfinite trace values')
    require(len(cpu['bits']) >= (LAYERS + 1) * WORDS and len(cpu['floats']) >= (LAYERS + 1) * WORDS,
            'CPU layer bounds')
    rows = []
    for layer in range(LAYERS):
        begin = layer * WORDS
        expected = {'bits': cpu['bits'][(layer+1)*WORDS:(layer+2)*WORDS],
                    'floats': cpu['floats'][(layer+1)*WORDS:(layer+2)*WORDS]}
        actual = {'bits': bits[begin:begin+WORDS], 'floats': values[begin:begin+WORDS]}
        changed, maximum, relative = difference(expected, actual, 0, WORDS)
        require(math.isfinite(maximum) and math.isfinite(relative), 'nonfinite comparison')
        rows.append((layer, changed, maximum, relative))
    return rows, next((row[0] for row in rows if row[3] > threshold), None)

def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('cpu', type=pathlib.Path, help='directory containing the three pinned original CPU controls')
    parser.add_argument('trace', type=pathlib.Path, help='raw 64 x 5120 binary32 values, position zero')
    parser.add_argument('--sha256', required=True, help='expected SHA-256 of this exact trace file')
    parser.add_argument('--relative-threshold', required=True, type=float, help='explicit diagnostic relative-L2 threshold')
    args = parser.parse_args(argv)
    require(args.cpu.is_absolute() and args.trace.is_absolute() and not args.trace.is_symlink(), 'absolute paths and nonsymlink trace required')
    require(not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ), 'role override')
    require(re.fullmatch(r'[0-9a-f]{64}', args.sha256) is not None, 'expected lowercase SHA-256 required')
    data = stable_bytes(args.trace, TRACE_BYTES)
    require(hashlib.sha256(data).hexdigest() == args.sha256, 'trace SHA-256 mismatch')
    cpu = load_cpu(args.cpu)[0]  # validates all pinned CPU controls and the fixed prompt
    rows, first = compare(cpu, data, args.relative_threshold)
    for layer, changed, maximum, relative in rows:
        print(f'layer={layer} bit_differences={changed} max_abs={maximum:.12g} relative_l2={relative:.12g}')
    print(f'FIRST_THRESHOLD_EXCEEDANCE={first if first is not None else "none"} threshold={args.relative_threshold:.12g}')
    print('RAW DATA ONLY: supplied hash does not prove source, completion, layer identity, device execution, correctness or speed')
    return 1 if first is not None else 0

if __name__ == '__main__':
    try:
        sys.exit(main())
    except (ValueError, OSError) as error:
        print('LAYER_TRACE_REFUSAL ' + str(error), file=sys.stderr)
        sys.exit(2)
