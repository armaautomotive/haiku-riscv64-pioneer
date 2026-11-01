#!/usr/bin/env python3
"""Read-only weight-traffic accounting,NOT a hardware/inference benchmark.

Uses the bounded existing GGUF metadata reader. Does not read/dequantize
weight payloads,authenticate the full file,open a device or change a model.
The scenario is ONE complete quantized-matrix weight sweep per decoded
token,excluding the embedding table. It is conditional accounting,not a
universal lower bound: caching,other layouts and batching change traffic.
"""
import argparse
import collections
import json
import math
import os
import struct

from tenstorrent_gguf_preflight import MAX_HEADER, inspect


def account(tensors):
    formats = {'Q8_0': (32, 34, 4), 'Q4_K': (256, 144, 2), 'Q6_K': (256, 210, 2)}
    result = {'quantized_matrices': 0, 'raw_weight_bytes': 0, 'tiled_weight_bytes': 0,
              'single_tile_commands': 0, 'batched_commands': 0, 'batch_last_tile_l1_copy_bytes': 0}
    empty = dict(result)
    groups = collections.defaultdict(lambda: dict(empty))
    storage_raw = storage_tiled = 0
    names = set()
    for tensor in tensors:
        name, shape, kind = tensor['name'], tensor['shape'], tensor['format']
        if not isinstance(name, str) or not name or name in names:
            raise ValueError('empty or duplicate tensor name')
        names.add(name)
        if kind == 'F32':
            continue  # norms/convolution/other learned bytes are NOT matrix traffic here
        if kind not in formats or len(shape) != 2:
            raise ValueError('expected F32 or a supported rank-2 quantized matrix')
        inputs, outputs = shape
        if any(type(v) is not int or not 0 < v < 2**32 for v in shape):
            raise ValueError('invalid dimensions')
        elements, block_bytes, batch = formats[kind]
        if inputs % elements or outputs % 16 or (name != 'token_embd.weight' and inputs // elements % batch):
            raise ValueError('matrix cannot use current complete-tile batch geometry')
        tiles = inputs // elements * (outputs // 16)
        tile_bytes = (16 * block_bytes + 63) // 64 * 64
        raw_bytes = inputs // elements * outputs * block_bytes
        if type(tensor['bytes']) is not int or tensor['bytes'] != raw_bytes:
            raise ValueError('quantized tensor length disagrees with dimensions')
        storage_raw += raw_bytes
        storage_tiled += tiles * tile_bytes
        if name == 'token_embd.weight':
            continue  # lookup is not a full-table sweep;lookup traffic is excluded too
        family = 'ffn' if name.startswith('blk.') and '.ffn_' in name else 'vocabulary_head' if name == 'output.weight' else 'projections'
        values = {'quantized_matrices': 1, 'raw_weight_bytes': raw_bytes, 'tiled_weight_bytes': tiles * tile_bytes,
                  'single_tile_commands': tiles, 'batched_commands': tiles // batch,
                  'batch_last_tile_l1_copy_bytes': tiles // batch * tile_bytes}
        for key, value in values.items():
            result[key] += value
            groups[family][key] += value
    if not result['quantized_matrices']:
        raise ValueError('no non-embedding quantized matrices')
    return {'scenario': 'ONE_SWEEP_QUANTIZED_MATRICES_EXCLUDING_EMBEDDING_NOT_MEASURED_TRAFFIC',
            'totals': result, 'families': dict(sorted(groups.items())),
            'all_quantized_storage_raw_bytes': storage_raw,
            'all_quantized_storage_tiled_bytes': storage_tiled,
            'limits': ['No full-file SHA authentication or weight-payload reads.',
                       'Excludes F32 weights,embedding lookup,K/V,recurrent state,activations,controller traffic and repetition.',
                       'No claim of hardware integrity,attainable bandwidth,model correctness or inference speed.',
                       'L1 last-tile copy bytes are NOT extra GDDR weight payload bytes.',
                       'No SRAM caching or cross-sequence weight reuse assumed;other implementations can differ.']}


def scenario_budget(accounting, target_tokens_per_second, nominal_gb_per_second):
    for value in (target_tokens_per_second, nominal_gb_per_second):
        if not math.isfinite(value) or value <= 0:
            raise ValueError('rates must be finite and positive')
    weight_bytes = accounting['totals']['tiled_weight_bytes']
    required = weight_bytes * target_tokens_per_second / 1e9
    if not math.isfinite(required) or not math.isfinite(required / nominal_gb_per_second):
        raise ValueError('scenario rate arithmetic overflow')
    return {'target_tokens_per_second': target_tokens_per_second,
            'assumed_nominal_decimal_gb_per_second': nominal_gb_per_second,
            'conditional_weight_payload_decimal_gb_per_second': required,
            'fraction_of_assumed_nominal_bandwidth_for_weight_payload_only': required / nominal_gb_per_second,
            'interpretation': 'CONDITIONAL_ONE_SWEEP_BUDGET_NOT_A_MEASURED_BANDWIDTH_OR_TOKEN_RATE_OR_IMPOSSIBILITY_PROOF'}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('model')
    parser.add_argument('--target-tokens-per-second', type=float, required=True)
    parser.add_argument('--nominal-gb-per-second', type=float, required=True,
                        help='explicit scenario input,not a detected card specification')
    args = parser.parse_args()
    try:
        # Read metadata from this SAME open description. A concurrent size,
        # identity or timestamp change invalidates even this layout report.
        with open(args.model, 'rb') as source:
            before = os.fstat(source.fileno())
            inventory = inspect(source.read(MAX_HEADER), before.st_size, include_tensors=True)
            after = os.fstat(source.fileno())
            if any(getattr(before, key) != getattr(after, key) for key in
                   ('st_dev', 'st_ino', 'st_size', 'st_mtime_ns', 'st_ctime_ns')):
                raise ValueError('source changed during metadata read')
        result = account(inventory['tensors'])
        result['metadata_only'] = {key: inventory[key] for key in ('metadata', 'tensor_count', 'declared_full_bytes', 'tensor_bytes')}
        result['bandwidth_scenario'] = scenario_budget(result, args.target_tokens_per_second, args.nominal_gb_per_second)
        print(json.dumps(result, indent=2, sort_keys=True))
    except (OSError, ValueError, UnicodeError, OverflowError, struct.error) as error:
        parser.exit(1, f'STOP: {error};NO_DEVICE_OR_MODEL_EXECUTION\n')


if __name__ == '__main__':
    main()
