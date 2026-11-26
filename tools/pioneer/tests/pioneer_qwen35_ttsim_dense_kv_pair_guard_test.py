#!/usr/bin/env python3
"""Reject K/V pair recorder images before the generic numerical runner starts."""
from pathlib import Path
import importlib.util
import subprocess
import sys
import tempfile

runner = Path(__file__).with_name('pioneer_qwen35_ttsim_service_test.py')
spec = importlib.util.spec_from_file_location('qwen35_ttsim_service_test', runner)
service = importlib.util.module_from_spec(spec)
spec.loader.exec_module(service)
assert service.recorded_graph_step_counts(True, True, True, True) == (425, 428)
assert service.recorded_graph_step_counts(False, True, True, True) == (433, 436)
assert service.recorded_graph_step_counts(False, False, True, True) == (497, 500)
assert service.recorded_graph_step_counts(False, False, False, True) == (529, 532)
assert service.recorded_graph_step_counts(False, False, False, False) == (521, 524)
with tempfile.TemporaryDirectory() as temporary:
    root = Path(temporary)
    image = root / 'image'
    image.mkdir()
    (image / 'dense-kv-pair.marker').touch()
    result = subprocess.run([sys.executable, '-B', str(runner), 'missing-library',
                             str(image), 'missing-roles'], capture_output=True, text=True)
    assert result.returncode != 0
    assert 'dense K/V pair image requires a matching recorded numerical fixture' in result.stderr
    for marker in ('ffn-pair.marker', 'flash-hybrid.marker', 'fused-rms-save.marker',
                   'compact-two-region.marker', 'activation-sharing.marker',
                   'delta-shard.marker', 'ttsim-unprofiled.marker'):
        (image / marker).touch()
    fixture = root / 'unpaired-fixture'
    fixture.mkdir()
    (fixture / 'templates.bin').write_bytes(bytes(120 * 512))
    for marker in ('ffn-pair.marker', 'flash-hybrid.marker', 'fused-rms-save.marker'):
        (fixture / marker).touch()
    reference = root / 'reference.bin'
    reference.touch()
    result = subprocess.run([
        sys.executable, '-B', str(runner), 'missing-library', str(image), 'missing-roles',
        '--ttsim-unprofiled', '--flash-hybrid', '--compact-two-region',
        '--recorded-layer-fixture', str(fixture), '--recorded-layer-reference', str(reference)],
        capture_output=True, text=True)
    assert result.returncode != 0
    assert 'dense K/V pair fixture and image markers do not match' in result.stderr
print('PASS K/V pair image rejected before numerical simulator setup; OFFLINE ONLY')
