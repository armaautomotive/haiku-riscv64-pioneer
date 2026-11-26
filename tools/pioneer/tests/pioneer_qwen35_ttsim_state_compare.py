#!/usr/bin/env python3
"""Compare audited simulator position-zero state to CPU position-one input state.

This is an offline diagnostic, not an inference fallback or hardware audit.
The two implementations use the same channel-major convolution history and
head/value/key-major delta-state layout. The caller must independently verify
that the simulator address map was extracted from the executed graph.
"""
import argparse
from array import array
import hashlib
import json
import math
from pathlib import Path
import sys


def compare_state(actual, reference, count):
    if len(actual) != count * 4 or len(reference) != count * 4:
        raise ValueError('unexpected recurrent state size')
    values, expected = array('f'), array('f')
    values.frombytes(actual);expected.frombytes(reference)
    if sys.byteorder != 'little':
        values.byteswap();expected.byteswap()
    failed = 0
    maximum = scaled_max = 0.0
    worst = 0
    for index, (got, want) in enumerate(zip(values, expected)):
        if not math.isfinite(got) or not math.isfinite(want):
            raise ValueError(f'non-finite recurrent state at {index}')
        error = abs(got - want)
        scaled = error / max(1.0, abs(want))
        maximum = max(maximum, error)
        if scaled > scaled_max:
            scaled_max, worst = scaled, index
        if scaled > 2e-5:
            failed += 1
    return {'rows': count, 'mismatches': failed, 'max_absolute_error': maximum,
            'max_scaled_error': scaled_max, 'worst_index': worst,
            'simulator_sha256': hashlib.sha256(actual).hexdigest(),
            'reference_sha256': hashlib.sha256(reference).hexdigest()}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('simulator', type=Path)
    parser.add_argument('reference', type=Path)
    parser.add_argument('layers', type=int)
    args = parser.parse_args()
    if not 1 <= args.layers <= 32 or not (args.simulator / 'worker-audit-complete.marker').is_file():
        parser.error('requires a complete simulator worker audit and 1..32 layers')
    checks = []
    for layer in range(args.layers):
        if layer % 4 == 3:
            continue
        for code, name, count in (('H', 'conv', 24576), ('D', 'delta', 524288)):
            actual = (args.simulator / f'{code}-layer{layer}.bin').read_bytes()
            reference = (args.reference / f'position1-{name}-state-{layer}.bin').read_bytes()
            result = compare_state(actual, reference, count)
            result.update(layer=layer, state=name)
            checks.append(result)
    passed = all(not result['mismatches'] for result in checks)
    print(json.dumps({'passed': passed, 'checks': checks,
                      'scope': 'simulated state against offline CPU reference; no hardware'}, indent=2))
    return 0 if passed else 1


if __name__ == '__main__':
    raise SystemExit(main())
