#!/usr/bin/env python3
"""Compare completed simulator prefix snapshots with independent references."""
import importlib.util
from pathlib import Path
import sys

if len(sys.argv) != 4:
    raise SystemExit('usage: compare snapshot-dir reference-dir layer-count')
snapshots, reference = map(Path, sys.argv[1:3])
try:
    layers = int(sys.argv[3])
except ValueError:
    raise SystemExit('layer count must be an integer')
if not 2 <= layers <= 32 or not (snapshots / 'execution-complete.marker').is_file():
    raise SystemExit('a completed recorded run is required')
path = Path(__file__).with_name('pioneer_qwen35_ttsim_service_test.py')
spec = importlib.util.spec_from_file_location('q35_service', path)
service = importlib.util.module_from_spec(spec)
spec.loader.exec_module(service)
failed = False
for layer in range(layers):
    actual = (snapshots / f'layer{layer}.bin').read_bytes()
    expected = (reference / f'layer{layer}.bin').read_bytes()
    try:
        error = service.compare_recorded_layer(actual, expected)
        print(f'layer {layer}: PASS; maximum absolute error {error:.9g}')
    except RuntimeError as error:
        failed = True
        print(f'layer {layer}: FAIL; {error}')
print('simulator-only comparison; NO HARDWARE')
raise SystemExit(int(failed))
