#!/usr/bin/env python3
"""Offline, same-input CPU checks for a completed simulator layer snapshot.

This launches the standalone authenticated llama.cpp oracle. It never supplies
inference to Work mode, touches the Pioneer card, or establishes throughput.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import struct
import subprocess

from pioneer_qwen35_ttsim_service_test import (RecordedNumericalMismatch, compare_recorded_layer,
                                                recorded_prefix_snapshot_points)


def read_finite(path):
    data = path.read_bytes()
    if len(data) != 2560 * 4 or any(not math.isfinite(x[0]) for x in struct.iter_unpack('<f', data)):
        raise ValueError(f'invalid layer tensor: {path}')
    return data


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('oracle', type=Path)
    parser.add_argument('model', type=Path)
    parser.add_argument('snapshots', type=Path)
    parser.add_argument('fixture', type=Path)
    parser.add_argument('reference', type=Path)
    parser.add_argument('output', type=Path)
    parser.add_argument('--layers', type=int, required=True)
    parser.add_argument('--actual-token', type=int,
                        help='candidate token from the separately audited full simulator run')
    args = parser.parse_args()
    if not 2 <= args.layers <= 32:
        parser.error('layer count must be between 2 and 32')
    if args.actual_token is not None and (args.layers != 32 or not 0 <= args.actual_token < 248320):
        parser.error('an actual token is valid only for a full decode')
    if args.layers == 32 and args.actual_token is None:
        parser.error('a full decode requires the separately audited candidate token')
    if not (args.snapshots / 'execution-complete.marker').is_file():
        parser.error('simulator execution marker is missing')
    if args.output.exists() or args.output.is_symlink():
        parser.error('output directory must not already exist')
    command = (args.fixture / 'command.bin').read_bytes()
    if len(command) != 128 or struct.unpack_from('<2I', command, 32) != (42, 0):
        parser.error('fixture must use token 42 at position zero')
    layout_bytes = (args.fixture / 'layout.txt').read_bytes()
    configs = [line.split() for line in layout_bytes.decode('utf-8').splitlines()
               if line.startswith('C ')]
    if len(configs) != 1 or len(configs[0]) not in (4, 5):
        parser.error('fixture must have one complete layout configuration')
    values = list(map(int, configs[0][1:]))
    layers, executed, record_count = values[:3]
    decode = values[3] if len(values) == 4 else 0
    if layers != args.layers or decode != int(layers == 32):
        parser.error('fixture layer count or decode flag differs')
    records = (args.fixture / 'records.bin').read_bytes()
    recorded_prefix_snapshot_points(records, record_count, executed, layers)
    tensors = [read_finite(args.snapshots / f'layer{i}.bin') for i in range(args.layers)]
    hashes = [hashlib.sha256(data).hexdigest() for data in tensors]
    first_reference = read_finite(args.reference / 'layer0.bin')
    args.output.mkdir()
    report = {'layers': args.layers, 'snapshot_sha256': hashes, 'checks': [],
              'oracle_executable_sha256': hashlib.sha256(args.oracle.read_bytes()).hexdigest(),
              'fixture_command_sha256': hashlib.sha256(command).hexdigest(),
              'fixture_layout_sha256': hashlib.sha256(layout_bytes).hexdigest(),
              'fixture_records_sha256': hashlib.sha256(records).hexdigest(),
              'candidate_token': args.actual_token, 'token_matches': None,
              'scope': 'offline simulator comparison only; no hardware or Work inference'}

    def compare(label, actual, expected):
        try:
            maximum = compare_recorded_layer(actual, expected)
            result = {'label': label, 'passed': True, 'max_absolute_error': maximum}
        except RecordedNumericalMismatch as error:
            result = {'label': label, 'passed': False, 'detail': str(error)}
        report['checks'].append(result)
        print(f"{label}: {'PASS' if result['passed'] else 'FAIL'}; "
              f"{result.get('max_absolute_error', result.get('detail'))}", flush=True)

    compare('initial layer 0', tensors[0], first_reference)
    # Each next layer receives exactly the preceding simulator activation.
    # Position zero starts with empty per-layer recurrent and KV state.
    for boundary in range(args.layers - 1 + int(args.actual_token is not None)):
        directory = args.output / f'boundary-{boundary:02d}'
        directory.mkdir()
        with (args.output / f'boundary-{boundary:02d}.log').open('x') as log:
            completed = subprocess.run([str(args.oracle), str(args.model), str(directory),
                                        '--replace-layer', str(boundary),
                                        str(args.snapshots / f'layer{boundary}.bin')],
                                       stdout=log, stderr=subprocess.STDOUT, check=False)
        if completed.returncode:
            raise RuntimeError(f'oracle failed for boundary {boundary}: {completed.returncode}')
        if read_finite(directory / f'layer{boundary}.bin') != tensors[boundary]:
            raise RuntimeError(f'oracle did not retain the exact boundary {boundary}')
        if boundary + 1 < args.layers:
            compare(f'layer {boundary + 1} from simulator layer {boundary}',
                    tensors[boundary + 1], read_finite(directory / f'layer{boundary + 1}.bin'))
        else:
            token = int((directory / 'token.txt').read_text().strip())
            report['oracle_token'] = token
            report['token_matches'] = token == args.actual_token
            print(f"same-final-hidden token: {'PASS' if report['token_matches'] else 'FAIL'}; "
                  f'simulator {args.actual_token}, oracle {token}', flush=True)
    if any(hashlib.sha256(read_finite(args.snapshots / f'layer{i}.bin')).hexdigest() != hashes[i]
           for i in range(args.layers)):
        raise RuntimeError('simulator snapshot changed during oracle comparison')
    report['passed'] = all(check['passed'] for check in report['checks']) and report['token_matches'] is not False
    with (args.output / 'summary.json').open('x') as output:
        json.dump(report, output, indent=2, sort_keys=True)
        output.write('\n')
    print('offline simulator comparison only; NO HARDWARE', flush=True)
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
