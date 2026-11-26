#!/usr/bin/env python3
"""Safety checks for simulator-only resident prefix observations."""
import importlib.util
from pathlib import Path
import struct
import subprocess
import sys
import tempfile

path = Path(__file__).with_name('pioneer_qwen35_ttsim_service_test.py')
spec = importlib.util.spec_from_file_location('q35_service', path)
service = importlib.util.module_from_spec(spec)
spec.loader.exec_module(service)

count = 6
records = bytearray(120 * count * 2048)

def put(core, step, mode):
    offset = (core * count + step) * 2048
    struct.pack_into('<6I', records, offset, 0, 0, 0, step + 1, core, 0x31535235)
    struct.pack_into('<4I', records, offset + 64, 0x31535235, 1, core, mode)
    struct.pack_into('<8I', records, offset + 64 + 20 * 4,
                     9216, 4, 1, 16, 0, 0x05000000, 0, 9216)
    struct.pack_into('<8I', records, offset + 64 + 52 * 4,
                     2560, 4, 1, 16, 0, 0x06000000, 0, 2560)

for core in range(120):
    for step, mode in enumerate((2, 2, 1, 2, 2)):
        put(core, step, mode)

points = service.recorded_prefix_snapshot_points(records, count, 5, 2)
assert points == [(2, [(0, 0x05000000, 0, 9216)])]
for step, phase, result in ((1, 5, 'waiting'), (2, 1, 'safe'), (2, 2, 'safe'),
                            (2, 3, 'safe'), (2, 4, 'missed'), (3, 1, 'missed')):
    assert service.recorded_snapshot_progress(0x51350000 | (step << 3) | phase, 2) == result
for step, expected in ((10, 100000), (15, 1000), (16, 50), (17, 50)):
    assert service.recorded_snapshot_clock(0x51350000 | (step << 3) | 1, 17) == expected

for progress in (0, 0x51350000 | (2 << 3), 0x51350000 | (2 << 3) | 6):
    try:
        service.recorded_snapshot_progress(progress, 2)
    except ValueError:
        pass
    else:
        raise AssertionError('invalid progress accepted')

for core, step, offset, value in ((70, 1, 16, 71), (70, 2, 64 + 12, 2),
                                   (70, 2, 64 + 20 * 4 + 20, 0x07000000),
                                   (70, 2, 64 + 52 * 4 + 20, 0x05000000)):
    changed = bytearray(records)
    struct.pack_into('<I', changed, (core * count + step) * 2048 + offset, value)
    try:
        service.recorded_prefix_snapshot_points(changed, count, 5, 2)
    except ValueError:
        pass
    else:
        raise AssertionError('malformed snapshot boundary accepted')

fused_records = bytearray(records)
for core in range(120):
    offset = (core * count + 2) * 2048
    struct.pack_into('<I', fused_records, offset + 20, 0x31534635)
    struct.pack_into('<3I', fused_records, offset + 64, 0x31534635, 1, core)
    struct.pack_into('<7I', fused_records, offset + 64 + 12, 0x314d5235, 1, 1, 0, core, 0, 1)
    struct.pack_into('<5I', fused_records, offset + 64 + 528, 0x31535235, 1, core, 1, 0)
    for word, rows, address in ((26, 9216, 0x05000000), (90, 9216, 0x07000000),
                                (152, 9216, 0x05000000), (184, 2560, 0x06000000)):
        struct.pack_into('<8I', fused_records, offset + 64 + word * 4,
                         rows, 4, 1, 16, 0, address, 0, rows)
assert service.recorded_prefix_snapshot_points(fused_records, count, 5, 2) == points
for offset, value in ((64 + 12 + 16, 71), (64 + 528 + 12, 2),
                      (64 + 90 * 4 + 20, 0x05000000),
                      (64 + 184 * 4 + 20, 0x05000000)):
    changed = bytearray(fused_records)
    struct.pack_into('<I', changed, (70 * count + 2) * 2048 + offset, value)
    try:
        service.recorded_prefix_snapshot_points(changed, count, 5, 2)
    except ValueError:
        pass
    else:
        raise AssertionError('malformed fused snapshot boundary accepted')

compare = path.with_name('pioneer_qwen35_ttsim_prefix_snapshot_compare.py')
with tempfile.TemporaryDirectory() as temporary:
    root = Path(temporary); actual = root / 'actual'; expected = root / 'reference'
    actual.mkdir(); expected.mkdir()
    for layer in range(2):
        for directory in (actual, expected):
            (directory / f'layer{layer}.bin').write_bytes(bytes(2560 * 4))
    def run():
        return subprocess.run([sys.executable, '-B', str(compare), str(actual), str(expected), '2'],
                              capture_output=True, text=True)
    assert run().returncode != 0
    (actual / 'execution-complete.marker').touch()
    assert run().returncode == 0
    (actual / 'layer0.bin').write_bytes(struct.pack('<f', 1.0) + bytes(2559 * 4))
    result = run()
    assert result.returncode == 1 and 'layer 0: FAIL' in result.stdout and 'layer 1: PASS' in result.stdout

print('PASS simulator prefix snapshot boundary and progress rejection checks; NO HARDWARE')
