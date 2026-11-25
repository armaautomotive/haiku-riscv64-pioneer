#!/usr/bin/env python3
"""Compose actual resident workers and fleet audit using simulated NoC."""
import os
from pathlib import Path
import subprocess
import tempfile

test_dir = Path(__file__).resolve().parent
marker = '\nint main() {'
service = (test_dir / 'pioneer_qwen35_resident_service_test.cpp').read_text()
fleet = (test_dir / 'pioneer_qwen35_resident_fleet_audit_test.cpp').read_text()
assert service.count(marker) == fleet.count(marker) == 1
with tempfile.TemporaryDirectory() as temporary:
    tmp = Path(temporary)
    (tmp / 'resident-service-run-fixture.inc').write_text(service.split(marker, 1)[0] + '\n')
    (tmp / 'resident-fleet-fixture.inc').write_text(fleet.split(marker, 1)[0] + '\n')
    for name, flags in (('ordinary', []),
                        ('paired', ['-DPIONEER_QWEN35_SHARED_ROLE_GRAPH_WITNESS', '-DPIONEER_QWEN35_PAIRED_RECURRENCE'])):
        binary = tmp / name
        subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra',
                        '-Werror', '-fsanitize=address,undefined', '-I' + str(tmp), '-I' + str(test_dir),
                        *flags, str(test_dir / 'pioneer_qwen35_resident_sender_fleet_test.cpp'),
                        '-o', str(binary)], check=True)
        env = dict(os.environ, ASAN_OPTIONS='quarantine_size_mb=32', UBSAN_OPTIONS='halt_on_error=1')
        subprocess.run([str(binary)], check=True, env=env)
