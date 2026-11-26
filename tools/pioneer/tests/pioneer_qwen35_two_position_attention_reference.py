#!/usr/bin/env python3
"""Independent float attention check for one warm simulated dense layer.

Inputs are offline traces, including CPU-normalized/rotated Q and K computed
from the simulator's injected projections. The previous KV is captured from
the simulator. This is not Work inference or a hardware/performance test.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import struct


def floats(path, count):
    raw = path.read_bytes()
    if len(raw) != count * 4:
        raise ValueError(f'{path}: unexpected float tensor size')
    values = struct.unpack(f'<{count}f', raw)
    if any(not math.isfinite(value) for value in values):
        raise ValueError(f'{path}: non-finite tensor')
    return raw, values


def half_round(values):
    try:
        packed = struct.pack(f'<{len(values)}e', *values)
    except (OverflowError, struct.error) as error:
        raise ValueError('tensor cannot be represented as F16') from error
    return struct.unpack(f'<{len(values)}e', packed)


def two_position_float_context(query, key, value, gate, prior):
    if (len(query), len(key), len(value), len(gate), len(prior)) != (4096, 1024, 1024, 4096, 2048):
        raise ValueError('unexpected attention tensor dimensions')
    if any(not math.isfinite(item) for row in (query, key, value, gate, prior) for item in row):
        raise ValueError('non-finite attention input')
    if any(item < 0.0 or item > 1.0 for item in gate):
        raise ValueError('sigmoid gate out of range')
    key = half_round(key);value = half_round(value)
    result = []
    for head in range(16):
        offset = (head // 4) * 256
        q = query[head * 256:(head + 1) * 256]
        k0, k1 = prior[offset:offset + 256], key[offset:offset + 256]
        scores = [sum(q[i] * row[i] for i in range(256)) / 16.0 for row in (k0, k1)]
        if any(not math.isfinite(score) for score in scores):
            raise ValueError('non-finite attention score')
        maximum = max(scores)
        probabilities = [math.exp(score - maximum) for score in scores]
        denominator = sum(probabilities)
        first, second = (probability / denominator for probability in probabilities)
        v0 = prior[1024 + offset:1024 + offset + 256]
        v1 = value[offset:offset + 256]
        result.extend((first * v0[i] + second * v1[i]) * gate[head * 256 + i] for i in range(256))
    return struct.pack('<4096f', *result)


def compare(actual, reference):
    if len(actual) != 4096 * 4 or len(reference) != 4096 * 4:
        raise ValueError('unexpected attention output size')
    maximum = scaled_maximum = 0.0
    mismatches = 0
    worst = 0
    for i, ((got,), (want,)) in enumerate(zip(struct.iter_unpack('<f', actual), struct.iter_unpack('<f', reference))):
        if not math.isfinite(got) or not math.isfinite(want):
            raise ValueError('non-finite attention output')
        difference = abs(got - want)
        scaled = difference / max(1.0, abs(want))
        maximum = max(maximum, difference)
        if scaled > scaled_maximum:
            scaled_maximum, worst = scaled, i
        mismatches += scaled > 2e-5
    return {'mismatches': mismatches, 'max_absolute_error': maximum,
            'max_scaled_error': scaled_maximum, 'worst_index': worst}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('trace', type=Path)
    parser.add_argument('prior_kv', type=Path)
    parser.add_argument('simulator_context', type=Path)
    parser.add_argument('cpu_context', type=Path)
    args = parser.parse_args()
    names = (('Qcur', 4096), ('Kcur', 1024), ('Vcur_projected', 1024), ('gate_sigmoid', 4096))
    traced = [floats(args.trace / f'trace-{name}-3.bin', count) for name, count in names]
    prior_raw = args.prior_kv.read_bytes()
    if len(prior_raw) != 4096:
        raise ValueError('unexpected prior KV size')
    prior = struct.unpack('<2048e', prior_raw)
    reference = two_position_float_context(*(values for _, values in traced), prior)
    simulator = args.simulator_context.read_bytes()
    cpu = args.cpu_context.read_bytes()
    result = {'simulator_vs_float': compare(simulator, reference),
              'cpu_flash_vs_float': compare(cpu, reference),
              'input_sha256': {name: hashlib.sha256(raw).hexdigest() for (name, _), (raw, _) in zip(names, traced)},
              'prior_kv_sha256': hashlib.sha256(prior_raw).hexdigest(),
              'simulator_sha256': hashlib.sha256(simulator).hexdigest(),
              'cpu_sha256': hashlib.sha256(cpu).hexdigest(),
              'float_reference_sha256': hashlib.sha256(reference).hexdigest(),
              'scope': 'two-position float attention using offline CPU traces and simulated prior KV; no hardware'}
    print(json.dumps(result, indent=2))
    return 0 if result['simulator_vs_float']['mismatches'] == 0 else 1


if __name__ == '__main__':
    raise SystemExit(main())
