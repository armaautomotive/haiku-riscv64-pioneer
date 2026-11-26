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
