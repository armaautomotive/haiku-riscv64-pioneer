#!/usr/bin/env python3
"""Read-only first-token prepared/NEXT-row diagnostic;NO device/native IO."""
import argparse
import json
import math
import pathlib
import re
import runpy
import sys

raw_tools = runpy.run_path(str(pathlib.Path(__file__).with_name('analyze_qgt52_raw_context.py')))
MARKER = 'DEBUG_CPU_PREPARED_STATE_CONTEXT_FROM_EXISTING_REPLY_ONLY_PARTNER_MAY_BE_UNCHECKED;NO_DEVICE_IO_OR_NATIVE_GRANT'
ROW = re.compile(r'DEBUG_CPU_(EXPECTED_PREPARED_[01]|ACTUAL_PREPARED_[01]|EXPECTED_NEXT_ROW|ACTUAL_NEXT_ROW) word(\d{3})((?: [0-9a-f]{8}){2,8})')
SIZES = {**{side+'_PREPARED_'+str(half): 322 for side in ('EXPECTED', 'ACTUAL') for half in (0, 1)},
         'EXPECTED_NEXT_ROW': 128, 'ACTUAL_NEXT_ROW': 128}
PREFIXES = ('DEBUG_CPU_EXPECTED_PREPARED', 'DEBUG_CPU_ACTUAL_PREPARED', 'DEBUG_CPU_EXPECTED_NEXT_ROW', 'DEBUG_CPU_ACTUAL_NEXT_ROW')


def drift(expected, actual):
    """Bounded CPU-log comparison only;not a validation tolerance or grant."""
    if not expected or len(expected) != len(actual):
        raise ValueError('require equally sized nonempty diagnostic vectors')
    number = raw_tools['number']
    def ordered(word):
        magnitude = word & 0x7fffffff
        return 0x80000000-magnitude if word & 0x80000000 else 0x80000000+magnitude
    differences, zero_differences = 0, 0
    max_ulp, max_absolute, max_relative = 0, 0.0, 0.0
    worst_ulp, worst_relative = None, None
    for index, (left, right) in enumerate(zip(expected, actual)):
        a, b = number(left), number(right)
        if not math.isfinite(a) or not math.isfinite(b):
            raise ValueError('nonfinite diagnostic drift input')
        differences += left != right
        zero_differences += a == 0 and b != 0
        ulp, absolute = abs(ordered(left)-ordered(right)), abs(a-b)
        if ulp > max_ulp:
            max_ulp, worst_ulp = ulp, index
        max_absolute = max(max_absolute, absolute)
        if a != 0 and absolute/abs(a) > max_relative:
            max_relative, worst_relative = absolute/abs(a), index
    return {'different_bits': differences, 'max_ulp': max_ulp, 'worst_ulp_index': worst_ulp,
            'max_absolute': max_absolute, 'max_relative_nonzero_reference': max_relative,
            'worst_relative_index': worst_relative, 'nonzero_against_zero_reference': zero_differences}


def parse(text):
    context = raw_tools['parse'](text)
    rows = {name: {} for name in SIZES}
    markers = 0
    for line in text.splitlines():
        if line.startswith('DEBUG_CPU_PREPARED_STATE_CONTEXT'):
            if line != MARKER:
                raise ValueError('malformed prepared-context marker')
            markers += 1
        elif line.startswith(PREFIXES):
            match = ROW.fullmatch(line)
            if not match:
                raise ValueError('malformed prepared/NEXT-row record')
            name, offset, words = match.groups()
            offset, values = int(offset), [int(v, 16) for v in words.split()]
            if offset % 8 or offset >= SIZES[name] or offset in rows[name] or len(values) != min(8, SIZES[name]-offset):
                raise ValueError('duplicate/out-of-range/truncated prepared/NEXT row')
            rows[name][offset] = values
    if markers != 1:
        raise ValueError('require ONE complete prepared/NEXT context')
    for name, parts in rows.items():
        if set(parts) != set(range(0, SIZES[name], 8)):
            raise ValueError('incomplete prepared/NEXT context')
        context[name] = [value for at in range(0, SIZES[name], 8) for value in parts[at]]
    return context


def analyze(context):
    number, f32, bits = (raw_tools[name] for name in ('number', 'f32', 'bits'))
    raw_report = raw_tools['analyze'](context)  # independently bounded raw/Z input domain
    values = {}
    for name in SIZES:
        values[name] = [number(v) for v in context[name]]
        for word, value in zip(context[name], values[name]):
            magnitude = word & 0x7fffffff
            if not math.isfinite(value) or magnitude > 0x477fe000 or 0 < magnitude < 0x00800000:
                raise ValueError('prepared/NEXT value outside bounded NORMAL-or-zero domain')
    half, lane = divmod(context['lane'], 64)
    result = {'diagnostic_only': True, 'log_authenticated': False, 'head': context['head'], 'lane': context['lane'],
              'observed_gated_relative_error': raw_report['observed_relative_error'],
              'assumptions': ['First-token CURRENT state is zero;log data is NOT source/owner authenticated.',
                              'Sequential binary32 multiply/add order,not a fused-path or physical-input proof.',
                              'Partner-half data may be unchecked;no numeric bound,reference or native grant is changed.']}
    scale = f32(0.0883883461356163)
    for side in ('EXPECTED', 'ACTUAL'):
        prepared = values[side+'_PREPARED_'+str(half)]
        if not (0 <= prepared[320] <= 1 and 0 <= prepared[321] <= 1):
            raise ValueError('decay/beta outside zero-to-one first-token domain')
        delta = f32(prepared[256+lane]*prepared[321])
        reconstructed = [f32(prepared[128+key]*delta) for key in range(128)]
        total, from_row = 0.0, 0.0
        for key in range(128):
            total = f32(total+f32(reconstructed[key]*prepared[key]))
            from_row = f32(from_row+f32(values[side+'_NEXT_ROW'][key]*prepared[key]))
        expected_raw = context[side+'_RAW'][context['lane']]
        result[side.lower()] = {
            'raw_from_prepared_bits': f'{bits(f32(total*scale)):08x}',
            'raw_from_next_row_bits': f'{bits(f32(from_row*scale)):08x}',
            'reported_raw_bits': f'{expected_raw:08x}',
            'next_row_different_from_prepared_words': sum(bits(v) != b for v, b in zip(reconstructed, context[side+'_NEXT_ROW'])),
            'shared_qk_differences_between_halves': sum(a != b for a, b in zip(context[side+'_PREPARED_0'][:256], context[side+'_PREPARED_1'][:256])),
            'next_row_vs_prepared_reconstruction': drift([bits(v) for v in reconstructed], context[side+'_NEXT_ROW']),
        }
    result['prepared_bit_differences'] = {
        name: sum(a != b for a, b in zip(context['EXPECTED_PREPARED_'+str(half)][start:end], context['ACTUAL_PREPARED_'+str(half)][start:end]))
        for name, start, end in (('q', 0, 128), ('k', 128, 256), ('v', 256, 320), ('decay', 320, 321), ('beta', 321, 322))}
    result['prepared_drift'] = {
        name: drift(context['EXPECTED_PREPARED_'+str(half)][start:end], context['ACTUAL_PREPARED_'+str(half)][start:end])
        for name, start, end in (('q', 0, 128), ('k', 128, 256), ('v', 256, 320), ('decay', 320, 321), ('beta', 321, 322))}
    result['next_row_drift'] = drift(context['EXPECTED_NEXT_ROW'], context['ACTUAL_NEXT_ROW'])
    result['raw_drift'] = drift(context['EXPECTED_RAW'], context['ACTUAL_RAW'])
    original, returned = (values[side+'_PREPARED_'+str(half)] for side in ('EXPECTED', 'ACTUAL'))
    # CPU-only counterfactuals isolate input sensitivity. These are neither
    # GPU measurements nor products offered to a native controller.
    def from_inputs(q, k, v, beta):
        total, delta = 0.0, f32(v*beta)
        for key in range(128):
            total = f32(total+f32(f32(k[key]*delta)*q[key]))
        return f'{bits(f32(total*scale)):08x}'
    result['prepared_counterfactual_raw_bits'] = {
        'q_only': from_inputs(returned[:128], original[128:256], original[256+lane], original[321]),
        'k_only': from_inputs(original[:128], returned[128:256], original[256+lane], original[321]),
        'v_only': from_inputs(original[:128], original[128:256], returned[256+lane], original[321]),
        'beta_only': from_inputs(original[:128], original[128:256], original[256+lane], returned[321]),
        'all_actual': from_inputs(returned[:128], returned[128:256], returned[256+lane], returned[321]),
    }
    result['qk_cancellation_condition_double_observer'] = {}
    for side, prepared in (('expected', original), ('actual', returned)):
        products = [prepared[key]*prepared[128+key] for key in range(128)]
        denominator = abs(math.fsum(products))
        result['qk_cancellation_condition_double_observer'][side] = math.fsum(map(abs, products))/denominator if denominator else None
    return result


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
        print('STOP_CPU_PREPARED_CONTEXT_ANALYSIS: '+str(error), file=sys.stderr)
        return 2
    print(json.dumps(report, indent=2, allow_nan=False))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
