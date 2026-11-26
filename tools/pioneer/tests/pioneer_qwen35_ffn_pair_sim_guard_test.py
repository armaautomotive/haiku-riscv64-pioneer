#!/usr/bin/env python3
"""Reject mismatched paired-FFN simulator image and fixture before loading ttsim."""
from pathlib import Path
import subprocess
import sys
import tempfile

runner = Path(__file__).with_name('pioneer_qwen35_ttsim_service_test.py')
with tempfile.TemporaryDirectory() as tmp:
    root = Path(tmp)
    image = root / 'image'; fixture = root / 'fixture'; roles = root / 'roles'
    for directory in (image, fixture, roles):
        directory.mkdir()
    (fixture / 'templates.bin').write_bytes(bytes(120 * 512))
    for marker in ('flash-hybrid.marker', 'ttsim-unprofiled.marker', 'activation-sharing.marker', 'delta-shard.marker'):
        (image / marker).touch()
    (fixture / 'flash-hybrid.marker').touch()
    base = [sys.executable, '-B', str(runner), str(root / 'no-library'), str(image), str(roles),
            '--ttsim-unprofiled', '--flash-hybrid', '--recorded-layer-fixture', str(fixture),
            '--recorded-layer-reference', str(root / 'no-reference')]
    for image_marker, fixture_marker in ((True, False), (False, True)):
        for directory, enabled in ((image, image_marker), (fixture, fixture_marker)):
            path = directory / 'ffn-pair.marker'
            if enabled:
                path.touch()
            else:
                path.unlink(missing_ok=True)
        result = subprocess.run(base, capture_output=True, text=True)
        assert result.returncode and 'paired FFN fixture and image markers do not match' in result.stderr
        assert 'no-library' not in result.stderr
    existing = root / 'existing-output'; existing.write_bytes(b'preserve')
    result = subprocess.run(base + ['--recorded-export-actual', str(existing)], capture_output=True, text=True)
    assert result.returncode and 'requires one fixture run and a new output path' in result.stderr
    assert existing.read_bytes() == b'preserve'
print('PASS paired FFN image/fixture mismatch rejected before simulator load; NO HARDWARE')
