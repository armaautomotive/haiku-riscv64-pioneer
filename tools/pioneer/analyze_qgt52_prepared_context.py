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
        }
    result['prepared_bit_differences'] = {
        name: sum(a != b for a, b in zip(context['EXPECTED_PREPARED_'+str(half)][start:end], context['ACTUAL_PREPARED_'+str(half)][start:end]))
        for name, start, end in (('q', 0, 128), ('k', 128, 256), ('v', 256, 320), ('decay', 320, 321), ('beta', 321, 322))}
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
