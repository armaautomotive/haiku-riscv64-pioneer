#!/usr/bin/env python3
"""Offline provenance/hidden comparison for one completed fresh tag129 trial.

This accepts copied files, not device access. It does not prove that a copied
transcript itself originated on the card; independently match remote hashes.
"""
import hashlib
import os
from pathlib import Path
import re
import subprocess
import sys

from compare_pioneer_q38_observed_trace import CPU_PINS, decode_trace, require, stable_bytes
from compare_pioneer_q38_qgw8_hidden import CARD_BYTES, CPU_SNAPSHOT_PIN, decode_card, metrics, words32
from verify_pioneer_q38_qgw8_cpu_hidden_snapshot import SNAPSHOT_BYTES, TRACE_BYTES, WIDTH, inspect

def read_bounded(path, limit):
    before = path.lstat()
    require(not path.is_symlink() and 0 <= before.st_size <= limit, f'{path}: unsafe size or symlink')
    return stable_bytes(path, before.st_size)

def main():
    require(len(sys.argv) == 4, 'usage: script ABS_TRIAL_COPY ABS_PINNED_TRACE ABS_CPU_HIDDEN')
    require(not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ), 'unexpected role overrides')
    trial, trace, cpu = map(Path, sys.argv[1:])
    require(all(p.is_absolute() for p in (trial, trace, cpu)) and trial.is_dir() and not trial.is_symlink(), 'absolute paths required')
    require(read_bounded(trial/'state', 128) == b'CANDIDATE_CLOSED_STABLE_RESTORED_QUALITY_UNVERIFIED\n', 'trial not closed/restored')
    require(read_bounded(trial/'trial.exit', 16) == b'0\n' and read_bounded(trial/'trial.stderr', 1) == b'', 'trial exit/stderr')
    transcript = read_bounded(trial/'trial.stdout', 2*1024*1024)
    gate = Path(__file__).resolve().parent.parent/'pioneer_q38_qgw8_hidden_output.awk'
    gate_source = read_bounded(gate, 65536)
    require(hashlib.sha256(gate_source).hexdigest() == '28ce7db66468f521b727dccb81cb00aeea1dcfb84e605c6c415e863eeda3eccd', 'transcript gate hash')
    checked = subprocess.run(['awk', gate_source.decode('ascii')], input=transcript, capture_output=True, check=False)
    require(checked.returncode == 0 and not checked.stderr, 'trial transcript gate refused')
    shape = re.fullmatch(rb'QGW8_HIDDEN_SHAPE_CLEAN outputs=([01]) token=(-?[0-9]+) sha256=([0-9a-f]{64});NUMERICS_NOT_CHECKED\n', checked.stdout)
    require(shape is not None, 'trial transcript result shape')
    outputs, token, digest = int(shape[1]), int(shape[2]), shape[3].decode('ascii')
    require((outputs == 0 and token == -1) or (outputs == 1 and 0 <= token < 248320), 'trial token')
    card = read_bounded(trial/'hidden.bin', CARD_BYTES)
    actual, generation = decode_card(card, digest)
    require(generation == 5, 'fresh trial driver generation')
    trace_data = stable_bytes(trace, TRACE_BYTES)
    cpu_data = stable_bytes(cpu, SNAPSHOT_BYTES)
    require(hashlib.sha256(cpu_data).hexdigest() == CPU_SNAPSHOT_PIN, 'converted CPU snapshot hash')
    inspect(trace_data, cpu_data, CPU_PINS[0])
    values = words32(cpu_data[64:-4], 'f')
    results = []
    for name, reference in (('original', values[:WIDTH]), ('converted', values[WIDTH:])):
        relative, maximum, cosine = metrics(reference, actual)
        results.append(f'QGW8_HIDDEN_COMPARE reference={name} relative_l2={relative:.12g} '
                       f'max_abs={maximum:.12g} cosine={cosine if cosine is not None else "undefined"}')
    # The original trace includes the embedding and all 64 layer boundaries.
    # Similarity is only a stale-state clue: a match to an earlier layer is
    # not proof that the card stopped there, and QGW8 uses converted weights.
    original = decode_trace(trace_data, 0, False)
    card_words = words32(card[64:-4], 'I')
    candidates = []
    for layer in range(65):
        begin = layer * WIDTH
        relative, _, cosine = metrics(original['floats'][begin:begin+WIDTH], actual)
        exact = original['bits'][begin:begin+WIDTH] == card_words
        candidates.append((layer, relative, cosine, exact))
    nearest = min(candidates, key=lambda item: (item[1], item[0]))
    defined = [item for item in candidates if item[2] is not None]
    aligned = max(defined, key=lambda item: (item[2], -item[0])) if defined else None
    exact_layers = ','.join(str(item[0]) for item in candidates if item[3]) or 'none'
    for line in results: print(line)
    print(f'QGW8_HIDDEN_LAYER_SCAN nearest_original_completed_layers={nearest[0]} '
          f'relative_l2={nearest[1]:.12g} most_aligned_completed_layers={aligned[0] if aligned else "undefined"} '
          f'cosine={format(aligned[2], ".12g") if aligned else "undefined"} exact_original_layers={exact_layers};'
          'SIMILARITY_ONLY_NOT_FAILING_LAYER_OR_PROVENANCE')
    print(f'QGW8_TRIAL_DATA outputs={outputs} token={token} snapshot_sha256={digest} '
          f'transcript_sha256={hashlib.sha256(transcript).hexdigest()} generation={generation};'
          'OFFLINE_ONLY_NOT_CARD_PROVENANCE_OR_QUALITY_OR_SPEED')

if __name__ == '__main__':
    try:
        main()
    except (ValueError, OSError) as error:
        print(f'ERROR: {error}', file=sys.stderr)
        sys.exit(1)
