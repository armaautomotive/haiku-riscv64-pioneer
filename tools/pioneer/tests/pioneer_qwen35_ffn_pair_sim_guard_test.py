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
    (image / 'profiled-compile-only.marker').touch()
    result = subprocess.run(base, capture_output=True, text=True)
    assert result.returncode and 'profiled compact image is compile-only' in result.stderr
    assert 'no-library' not in result.stderr
    (image / 'profiled-compile-only.marker').unlink()
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
    for directory in (image, fixture):
        (directory / 'ffn-pair.marker').touch()
    for image_marker, fixture_marker in ((True, False), (False, True)):
        for directory, enabled in ((image, image_marker), (fixture, fixture_marker)):
            path = directory / 'fused-rms-save.marker'
            if enabled:
                path.touch()
            else:
                path.unlink(missing_ok=True)
        result = subprocess.run(base, capture_output=True, text=True)
        assert result.returncode and 'fused RMS/save fixture and image markers do not match' in result.stderr
        assert 'no-library' not in result.stderr
    for directory in (image, fixture):
        (directory / 'fused-rms-save.marker').touch()
    compact_marker = image / 'compact-two-region.marker'
    for flag, marker in ((True, False), (False, True)):
        if marker:
            compact_marker.touch()
        else:
            compact_marker.unlink(missing_ok=True)
        result = subprocess.run(base + (['--compact-two-region'] if flag else []), capture_output=True, text=True)
        assert result.returncode and 'compact two-region simulator flag and image marker do not match' in result.stderr
        assert 'no-library' not in result.stderr
    compact_marker.touch()
    (image / 'flash.bin').write_bytes(bytes(4))
    result = subprocess.run(base + ['--compact-two-region'], capture_output=True, text=True)
    assert result.returncode and 'compact image requires fused hybrid markers and no extra code image' in result.stderr
    assert 'no-library' not in result.stderr
    (image / 'flash.bin').unlink()
    compact_marker.unlink()
    for directory in (image, fixture):
        (directory / 'fused-rms-save.marker').unlink(missing_ok=True)
    existing = root / 'existing-output'; existing.write_bytes(b'preserve')
    result = subprocess.run(base + ['--recorded-export-actual', str(existing)], capture_output=True, text=True)
    assert result.returncode and 'requires one fixture run and a new output path' in result.stderr
    assert existing.read_bytes() == b'preserve'
    (fixture / 'layer-slice.marker').touch()
    result = subprocess.run(base, capture_output=True, text=True)
    assert result.returncode and 'requires a marked fixture and its input' in result.stderr
    (fixture / 'layer-slice.marker').unlink()
    result = subprocess.run(base + ['--recorded-layer-input', str(root / 'input')], capture_output=True, text=True)
    assert result.returncode and 'requires a marked fixture and its input' in result.stderr
    output_flags = ('--recorded-wide-output', '--recorded-qkv-output',
                    '--recorded-z-output', '--recorded-scalar-output')
    for flag in output_flags:
        result = subprocess.run(base + [flag], capture_output=True, text=True)
        assert result.returncode and 'requires an isolated simulator-only layer slice' in result.stderr
        assert 'no-library' not in result.stderr
    (fixture / 'layer-slice.marker').touch()
    for left in range(len(output_flags)):
        for right in range(left + 1, len(output_flags)):
            result = subprocess.run(base + ['--recorded-layer-input', str(root / 'input'),
                                           output_flags[left], output_flags[right]], capture_output=True, text=True)
            assert result.returncode and 'only one isolated output width may be selected' in result.stderr
            assert 'no-library' not in result.stderr
print('PASS paired FFN image/fixture mismatch rejected before simulator load; NO HARDWARE')
