#!/usr/bin/env python3
"""Distinct v3/v4 generation timing DATA; no execution/cleanup authority."""
from decimal import Decimal
import re
from parse_pioneer_q38_full_offload_log import NUMBER, require

PREFIX = 'FULL_OFFLOAD_PROGRESS id2 value4294967295 '
HEADER = re.compile(re.escape(PREFIX)+r'Full-card fast decode profile valid([01]) committed('+NUMBER+r') window_start('+NUMBER+r') last_position('+NUMBER+r') begin_cycles('+NUMBER+r') end_cycles('+NUMBER+r') state('+NUMBER+r') error('+NUMBER+r');AFTER_CLEAN_END,generation_layer0_through_final_STOP,not_host_counter_endpoints\.')
BUCKET = re.compile(re.escape(PREFIX)+r'Full-card fast decode profile bucket('+NUMBER+r') segments('+NUMBER+r') cycles('+NUMBER+r');AFTER_CLEAN_END,clock DATA,no frequency assumption\.')
SPLIT_HEADER = re.compile(HEADER.pattern.replace('fast decode profile', 'layer split profile', 1))
SPLIT_BUCKET = re.compile(BUCKET.pattern.replace('fast decode profile', 'layer split profile', 1))

def fast_decode_suffix(lines, cursor, committed, prompts, *, schema=3):
    require(type(schema) is int and schema in (3,4), 'explicit known generation timing schema')
    header = SPLIT_HEADER if schema==4 else HEADER
    bucket = SPLIT_BUCKET if schema==4 else BUCKET
    require(cursor < len(lines), 'explicit fast generation profile required')
    row = header.fullmatch(lines[cursor])
    require(row, f'distinct exact v{schema} fast generation header')
    valid, count, start, last, begin, end, state, error = map(int, row.groups())
    cursor += 1
    require(count == committed and max(start, last, state, error) <= 2**32-1 and max(begin, end) <= 2**64-1,
            'fast profile header bounds/committed mismatch')
    result = {'valid': bool(valid), 'schema_version': schema, 'committed': count, 'window_start': start,
              'last_position': last, 'begin_cycles': begin, 'end_cycles': end, 'state': state, 'error': error,
              'physical_clock_frequency_assumed': False, 'identical_to_host_counter_endpoints': False,
              'scope': 'first generation layer0 entry through final119 worker STOPs; FinalStop separate'}
    if not valid:
        return cursor, result
    require(start == prompts and 0 < start < count <= 8192 and last == count-1,
            'authenticated generation-only window, not prompt+generation counts')
    require(state == 2 and error == 0 and begin < end, 'closed valid fast generation timing')
    positions = count-start
    common = ('generation_entry', 'metadata')
    tail = ('head_normalize', 'head_matrix', 'feedback', 'state_commit', 'publication', 'final_stop')
    if schema==4:
        names = common+('recurrent_other', 'dense_other')+tail+(
            'matrix_total', 'conv_stage', 'gdn_stage', 'dense_attention_prepare_attention_kv')
        per_position = {1:64,2:480,3:144,10:496,11:48,12:48,13:16}
    else:
        names = common+('recurrent_outside_matrix_calls','dense_outside_matrix_calls')+tail+(
            'matrix_prepare','matrix_dispatch','matrix_gather_wait','matrix_finish')
        per_position = {1:64,2:432,3:128,10:496,11:496,12:496,13:496}
    total = 0
    buckets = []
    for phase, name in enumerate(names):
        require(cursor < len(lines), 'complete fourteen generation buckets required')
        row = bucket.fullmatch(lines[cursor])
        require(row, 'exact fast generation bucket')
        actual, segments, cycles = map(int, row.groups())
        cursor += 1
        expected = 1 if phase in (0, 9) else positions*per_position.get(phase, 1)
        require(actual == phase and segments == expected and 0 < cycles <= 2**64-1-total,
                'ordered generation-only counts and bounded cycles')
        total += cycles
        buckets.append({'phase': phase, 'name': name, 'segments': segments, 'cycles': cycles})
    require(total == end-begin, 'whole generation profile cycle sum')
    body = total-buckets[9]['cycles']
    require(body > 0, 'generation body excludes final STOP')
    for row in buckets:
        row['whole_profile_share_percent'] = str(Decimal(row['cycles'])*100/Decimal(total))
        row['generation_body_share_percent'] = None if row['phase'] == 9 else str(Decimal(row['cycles'])*100/Decimal(body))
    result.update(profiled_positions=positions, total_cycles=total, generation_body_cycles=body,
                  final_stop_cycles=buckets[9]['cycles'], buckets=buckets)
    return cursor, result
