#!/usr/bin/env python3
"""Pure independent small-geometry checks;NO hardware or inference."""
import copy
import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
from pioneer_q38_weight_budget import account, scenario_budget

tensors = [
    {'name': 'token_embd.weight', 'shape': [256, 16], 'format': 'Q4_K', 'bytes': 2304},
    {'name': 'blk.0.attn_q.weight', 'shape': [128, 16], 'format': 'Q8_0', 'bytes': 2176},
    {'name': 'blk.0.ffn_down.weight', 'shape': [512, 32], 'format': 'Q4_K', 'bytes': 9216},
    {'name': 'output.weight', 'shape': [512, 16], 'format': 'Q6_K', 'bytes': 6720},
    {'name': 'output_norm.weight', 'shape': [5120], 'format': 'F32', 'bytes': 20480}]
# Embedding's single block need not fill a matrix batch: storage lookup
# is NOT a full-table matrix job and cannot enter the sweep count.
actual = account(tensors)
assert actual['totals'] == {'quantized_matrices': 3, 'raw_weight_bytes': 18112, 'tiled_weight_bytes': 18304,
                            'single_tile_commands': 10, 'batched_commands': 4,
                            'batch_last_tile_l1_copy_bytes': 8576}
assert actual['all_quantized_storage_raw_bytes'] == 20416
assert actual['all_quantized_storage_tiled_bytes'] == 20608
assert actual['families']['projections']['tiled_weight_bytes'] == 2304
assert actual['families']['ffn']['tiled_weight_bytes'] == 9216
assert actual['families']['vocabulary_head']['tiled_weight_bytes'] == 6784
scenario = scenario_budget(actual, 20, 448)
assert scenario['conditional_weight_payload_decimal_gb_per_second'] == 0.00036608
assert scenario['fraction_of_assumed_nominal_bandwidth_for_weight_payload_only'] == 0.00036608 / 448
assert scenario['interpretation'].endswith('NOT_A_MEASURED_BANDWIDTH_OR_TOKEN_RATE_OR_IMPOSSIBILITY_PROOF')
refusals = 0
for index, key, value in ((1, 'format', 'F16'), (1, 'shape', [128, 17]), (1, 'shape', [64, 16]),
                           (1, 'shape', [True, 16]), (1, 'shape', [128]), (1, 'bytes', 2175),
                           (1, 'bytes', 2176.0), (1, 'name', 'output.weight')):
    bad = copy.deepcopy(tensors)
    bad[index][key] = value
    try:
        account(bad)
        raise AssertionError('accepted invalid matrix geometry')
    except ValueError:
        refusals += 1
for bad in ([], [tensors[0]], [tensors[-1]]):
    try:
        account(bad)
        raise AssertionError('accepted no matrix workload')
    except ValueError:
        refusals += 1
for value in (0, -1, float('inf'), float('nan')):
    for target, bandwidth in ((value, 448), (20, value)):
        try:
            scenario_budget(actual, target, bandwidth)
            raise AssertionError('accepted invalid rate')
        except ValueError:
            refusals += 1
try:
    scenario_budget(actual, 1e308, 448)
    raise AssertionError('accepted overflowed rate arithmetic')
except ValueError:
    refusals += 1
print(f'PASS weight budget independent Q8/Q4/Q6/padding/embedding-exclusion/family/copy/command arithmetic;'
      f'{refusals} refusals;CONDITIONAL_ACCOUNTING_NOT_HARDWARE_OR_INFERENCE')
