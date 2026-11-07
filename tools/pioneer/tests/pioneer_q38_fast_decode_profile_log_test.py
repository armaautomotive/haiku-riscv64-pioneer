#!/usr/bin/env python3
"""Synthetic whole transcript validation, not a model or speed result."""
import pioneer_q38_full_offload_rate_log_test as legacy
from parse_pioneer_q38_full_offload_rate_log import *

base = legacy.fixture()
count, prompts, begin = 51, 20, 1 << 40
positions = count-prompts
segments = [1]+[positions*v for v in (64, 432, 128, 1, 1, 1, 1, 1)]+[1]+[positions*496]*4
cycles = [n*19 for n in segments]
end = begin+sum(cycles)
header = PREFIX+f'Full-card fast decode profile valid1 committed51 window_start20 last_position50 begin_cycles{begin} end_cycles{end} state2 error0;AFTER_CLEAN_END,generation_layer0_through_final_STOP,not_host_counter_endpoints.'
buckets = [PREFIX+f'Full-card fast decode profile bucket{i} segments{n} cycles{c};AFTER_CLEAN_END,clock DATA,no frequency assumption.' for i, (n, c) in enumerate(zip(segments, cycles))]
startup = PREFIX+'Full-card startup timing vocab_us1200000 setup_us1500000 upload_calls20000 upload_bytes19399024640 upload_window_us800000 upload_ioctl_us600000 first_commit_valid1 first_commit_min_us1900000 first_commit_max_us2000000 answer_ready_us3900000;AFTER_CLEAN_END,host_us,first_commit_not_streamed_text,upload_window_includes_between_call_preparation.'
def insert(rows):
    lines = base.splitlines()
    return '\n'.join(lines[:-3]+rows+lines[-3:])+'\n'
opts = {'fast_decode_profile': True, 'startup_timing': True}
good = insert([header, *buckets, startup])
result = parse_complete(good, **opts)
p = result['on_card_fast_decode_profile']
assert p['valid'] and p['schema_version'] == 3 and p['profiled_positions'] == 31 and p['committed'] == 51
assert p['total_cycles'] == sum(cycles) and p['generation_body_cycles'] == sum(cycles)-cycles[9]
assert len(p['buckets']) == 14 and p['buckets'][9]['generation_body_share_percent'] is None
assert not p['identical_to_host_counter_endpoints'] and not p['physical_clock_frequency_assumed']
assert result['host_startup_observations']['available']
invalid = header.replace('valid1', 'valid0').replace('state2 error0', 'state3 error1')
assert not parse_complete(insert([invalid, startup]), **opts)['on_card_fast_decode_profile']['valid']
assert not parse_complete(insert([header, *buckets, STARTUP_UNAVAILABLE]), **opts)['host_startup_observations']['available']
refused = 0
def bad(text, **options):
    global refused
    try:
        parse_complete(text, **options)
    except (InvalidLog, ValueError):
        refused += 1
    else:
        raise AssertionError('accepted malformed/unselected fast generation transcript')
bad(good)
bad(good, phase_profile=True, bf16_profile=True)
for name in ('prepared_q4', 'phase_profile', 'bf16_profile'):
    bad(good, **{**opts, name: True})
for value in (None, 0, 1, 'true'):
    bad(good, fast_decode_profile=value, startup_timing=True)
for i in range(16):
    rows = [header, *buckets, startup]
    del rows[i]
    bad(insert(rows), **opts)
for i in range(14):
    for change in (buckets[i]+ '\n'+buckets[i], buckets[i].replace(f'bucket{i}', f'bucket{(i+1)%14}'),
                   buckets[i].replace(f'segments{segments[i]}', f'segments{segments[i]+1}'),
                   buckets[i].replace(f'cycles{cycles[i]}', 'cycles0'),
                   buckets[i].replace(f'cycles{cycles[i]}', f'cycles{cycles[i]+1}')):
        rows = buckets[:]
        rows[i] = change
        bad(insert([header, *rows, startup]), **opts)
    if i not in (0, 9):
        rows = buckets[:]
        rows[i] = rows[i].replace(f'segments{segments[i]}', f'segments{segments[i]//31*51}')
        bad(insert([header, *rows, startup]), **opts)
for old, new in (('window_start20', 'window_start19'), ('last_position50', 'last_position51'),
                 ('committed51', 'committed50'), ('state2', 'state3'), ('error0', 'error1'),
                 ('end_cycles'+str(end), 'end_cycles'+str(end+1)), ('begin_cycles'+str(begin), 'begin_cycles'+str(2**64)),
                 ('generation_layer0_through_final_STOP', 'post_prompt_counter_window')):
    bad(insert([header.replace(old, new), *buckets, startup]), **opts)
bad(insert([invalid, *buckets, startup]), **opts)
for at in range(0, len(good), 23):
    bad(good[:at], **opts)
assert parse_complete(base)['answer_hex'] == result['answer_hex']
print(f'PASS_FAST_DECODE_PROFILE_LOG v3/31of51_generation_positions/14exclusive_buckets/final_STOP_excluded_from_body/host_endpoints_distinct/invalid_timing_still_diagnostic/{refused}_mode_window_count_sum_order_truncation_refusals;SYNTHETIC_NO_HARDWARE_OR_SPEED')
