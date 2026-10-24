#!/usr/bin/env python3
"""Read-only CPU diagnostic for an existing QGT52 client log;NO device IO."""
import argparse
import json
import math
import pathlib
import re
import struct
import sys

FAILURE = re.compile(r'DEBUG_CPU_ONLY_FAILURE reason(relative_tolerance|absolute_tolerance|nonfinite_value|subnormal_flush) op000089de word(\d{1,6}) half(\d{1,2}) offset(\d{4}) expected_bits([0-9a-f]{8}) actual_bits([0-9a-f]{8}) reply_captured1;NO_ADDITIONAL_DEVICE_IO')
CONTEXT = re.compile(r'DEBUG_CPU_GATED_CONTEXT head(\d{1,2}) lane(\d{1,3});PAIRED_RAW_FROM_EXISTING_REPLY_ORIGINAL_Z_FROM_PREPARED_REFERENCE_NO_ADDITIONAL_DEVICE_IO')
ROW = re.compile(r'DEBUG_CPU_(EXPECTED_RAW|ACTUAL_RAW|ORIGINAL_Z) word(\d{3})((?: [0-9a-f]{8}){8})')


def number(bits):
    return struct.unpack('<f', struct.pack('<I', bits))[0]


def f32(value):
    try:
        return struct.unpack('<f', struct.pack('<f', value))[0]
    except OverflowError:
        return math.copysign(math.inf, value)


def bits(value):
    return struct.unpack('<I', struct.pack('<f', f32(value)))[0]


def parse(text):
    """Strict bounded data parser;refuse concatenated/partial/mixed contexts."""
    failures, contexts = [], []
    rows = {name: {} for name in ('EXPECTED_RAW', 'ACTUAL_RAW', 'ORIGINAL_Z')}
    for line in text.splitlines():
        if line.startswith('DEBUG_CPU_ONLY_FAILURE'):
            match = FAILURE.fullmatch(line)
            if not match:
                raise ValueError('missing exact gated numeric failure record')
            failures.append(match.groups())
        elif line.startswith('DEBUG_CPU_GATED_CONTEXT'):
            match = CONTEXT.fullmatch(line)
            if not match:
                raise ValueError('malformed context record')
            contexts.append(tuple(map(int, match.groups())))
        elif any(line.startswith('DEBUG_CPU_'+name) for name in rows):
            match = ROW.fullmatch(line)
            if not match:
                raise ValueError('malformed raw/Z row')
            name, offset, values = match.groups()
            offset = int(offset)
            if offset % 8 or offset >= 128 or offset in rows[name]:
                raise ValueError('duplicate/out-of-range raw/Z row')
            rows[name][offset] = [int(v, 16) for v in values.split()]
    if len(failures) != 1 or len(contexts) != 1:
        raise ValueError('require ONE complete first-failure context,not concatenated logs')
    reason, word, half, offset, expected, actual = failures[0]
    word, half, offset = int(word), int(half), int(offset)
    head, lane = contexts[0]
    if not (word < 829632 and half < 96 and 8578 <= offset < 8642 and head < 48 and lane < 128
            and divmod(word, 8642) == (half, offset) and head == half//2 and lane == (half % 2)*64+offset-8578):
        raise ValueError('inconsistent gated word/half/head/lane')
    result = {'reason': reason, 'word': word, 'head': head, 'lane': lane,
              'expected_bits': int(expected, 16), 'actual_bits': int(actual, 16)}
    for name, parts in rows.items():
        if set(parts) != set(range(0, 128, 8)):
            raise ValueError('incomplete raw/Z context')
        result[name] = [v for i in range(0, 128, 8) for v in parts[i]]
    return result


def inverse(raw):
    # Match the documented FP32 reduction order,not a binary64 reduction.
    total = 0.0
    for value in raw:
        total = f32(total+f32(value*value))
    argument = f32(f32(total*0.0078125)+f32(1e-6))
    value = f32(1.0/f32(math.sqrt(argument)))
    if not math.isfinite(value) or value <= 0:
        raise ValueError('invalid diagnostic inverse RMS')
    return value


def analyze(context):
    raw = [[number(v) for v in context[name]] for name in ('EXPECTED_RAW', 'ACTUAL_RAW', 'ORIGINAL_Z')]
    for words, values in zip((context[n] for n in ('EXPECTED_RAW', 'ACTUAL_RAW', 'ORIGINAL_Z')), raw):
        for word, value in zip(words, values):
            magnitude = word & 0x7fffffff
            if not math.isfinite(value) or magnitude > 0x477fe000 or 0 < magnitude < 0x00800000:
                raise ValueError('raw/Z context outside the qualified finite NORMAL-or-zero domain')
    expected_raw, actual_raw, z = raw
    expected, actual = number(context['expected_bits']), number(context['actual_bits'])
    if not math.isfinite(expected) or not math.isfinite(actual):
        raise ValueError('nonfinite gated output cannot support this drift estimate')
    lane = context['lane']
    ei, ai = inverse(expected_raw), inverse(actual_raw)
    report = {'diagnostic_only': True, 'log_authenticated': False, 'head': context['head'], 'lane': lane,
              'first_failure_word': context['word'], 'expected_gated': expected, 'actual_gated': actual,
              'expected_raw': expected_raw[lane], 'actual_raw': actual_raw[lane], 'original_z': z[lane],
              'expected_inverse_rms': ei, 'actual_inverse_rms': ai,
              'assumptions': ['Physical Z and learned norm are unchanged;actual Z is NOT in this log.',
                              'FP32 ordering and fused-path differences can contribute additional error.',
                              'Log data is untrusted diagnostic evidence,not a validated native producer or model proof.']}
    report['observed_relative_error'] = abs(actual-expected)/abs(expected) if expected else None
    if not expected_raw[lane]:
        report['estimate_available'] = False
        report['reason'] = 'zero original raw lane prevents ratio-based attribution'
        return report
    ratio = (actual_raw[lane]/expected_raw[lane])*(ai/ei)
    predicted = f32(expected*ratio)
    if not math.isfinite(predicted):
        raise ValueError('nonfinite drift estimate')
    report.update(estimate_available=True, raw_rms_ratio=ratio, predicted_gated_assuming_unchanged_z_norm=predicted,
                  predicted_gated_bits=f'{bits(predicted):08x}',
                  residual_relative_error_to_observed=abs(predicted-actual)/abs(actual) if actual else None)
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('log', type=pathlib.Path)
    args = parser.parse_args()
    try:
        with args.log.open('rb') as source:
            data = source.read(1024*1024+1)
        if len(data) > 1024*1024:
            raise ValueError('log exceeds bounded1MiB diagnostic input')
        report = analyze(parse(data.decode('utf-8')))
    except (OSError, UnicodeError, ValueError, OverflowError) as error:
        print('STOP_CPU_CONTEXT_ANALYSIS: '+str(error), file=sys.stderr)
        return 2
    print(json.dumps(report, indent=2, allow_nan=False))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
