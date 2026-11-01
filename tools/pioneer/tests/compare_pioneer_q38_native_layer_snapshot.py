#!/usr/bin/env python3
"""Strict posthoc INCOMPLETE native-layer DATA comparison;NO execution grant."""
import argparse
import array
import math
import os
import pathlib
import sys
from compare_pioneer_q38_observed_trace import difference, load_cpu, require, stable_bytes

def decode(data):
    require(len(data) == (16+5120+1)*4, 'incomplete native layer length')
    words = array.array('I')
    require(words.itemsize == 4, '32-bit host words')
    words.frombytes(data)
    if sys.byteorder != 'little':
        words.byteswap()
    h = words[:16]
    require(list(h[:2]) == [0x314c4e51, 1] and h[2] < 3 and h[3] < 248320
            and 1 <= h[4] <= 64 and h[5] == 5120 and not any(h[6:]),
            'incomplete native family/version/position/token/layers/size/reserved')
    require(words[-1] == 0x454e444c, 'incomplete native footer,NOT token terminal')
    floats = array.array('f')
    require(floats.itemsize == 4, 'binary32 host')
    floats.frombytes(data[64:-4])
    if sys.byteorder != 'little':
        floats.byteswap()
    require(all(math.isfinite(value) for value in floats), 'nonfinite incomplete native layer')
    return {'header': h, 'bits': words[16:-1], 'floats': floats}

def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('cpu', type=pathlib.Path)
    parser.add_argument('snapshot', type=pathlib.Path)
    args = parser.parse_args(argv)
    require(args.cpu.is_absolute() and args.snapshot.is_absolute() and not args.snapshot.is_symlink(),
            'absolute nonsymlink incomplete snapshot required')
    require(not any(key.startswith(('PIONEER_', 'TT_PIONEER_')) for key in os.environ), 'role override')
    cpu = load_cpu(args.cpu)  # ALL three original full-trace pins unchanged
    actual = decode(stable_bytes(args.snapshot, (16+5120+1)*4))
    h = actual['header']
    position, layers = h[2], h[4]
    require(args.snapshot.name == f'INCOMPLETE-native-position-{position}-layers-{layers}.bin', 'incomplete snapshot filename identity')
    require(h[3] == cpu[position]['header'][3], 'input path differs from independent original CPU trace')
    first, last = layers*5120, (layers+1)*5120
    expected = {'bits': cpu[position]['bits'][first:last], 'floats': cpu[position]['floats'][first:last]}
    changed, maximum, relative = difference(expected, actual, 0, 5120)
    print(f'COMPARE_INCOMPLETE_NATIVE_LAYER position{position}/completedLayers{layers}/words5120/'
          f'bitDifferences{changed}/maxAbs{maximum:.12g}/relativeL2{relative:.12g}')
    print(f'INCOMPLETE_NATIVE_LAYER_NUMERICAL_BITEXACT={int(changed == 0)} NO_TOLERANCE_USED;'
          'NOT token/terminal/STOP/END/full-model/physical/Work/TPS qualification')
    return 0 if changed == 0 else 1

if __name__ == '__main__':
    try:
        sys.exit(main())
    except (ValueError, OSError) as error:
        print('INCOMPLETE_NATIVE_LAYER_REFUSAL '+str(error), file=sys.stderr)
        sys.exit(2)
