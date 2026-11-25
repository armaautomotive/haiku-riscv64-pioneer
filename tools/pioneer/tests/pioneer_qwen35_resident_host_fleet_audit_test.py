#!/usr/bin/env python3
"""Compose the real host and fleet audit over simulated device state."""
import os
from pathlib import Path
import subprocess
import tempfile

test_dir = Path(__file__).resolve().parent
marker = '\nint main() {'
host = (test_dir / 'pioneer_qwen35_resident_host_pci_test.cpp').read_text()
fleet = (test_dir / 'pioneer_qwen35_resident_fleet_audit_test.cpp').read_text()
assert host.count(marker) == fleet.count(marker) == 1
with tempfile.TemporaryDirectory() as temporary:
    tmp = Path(temporary)
    (tmp / 'resident-host-fixture.inc').write_text(host.split(marker, 1)[0] + '\n')
    (tmp / 'resident-fleet-fixture.inc').write_text(fleet.split(marker, 1)[0] + '\n')
    for name, flags in (('ordinary', []), ('paired', ['-DPIONEER_QWEN35_PAIRED_RECURRENCE'])):
        binary = tmp / name
        subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra',
                        '-Werror', '-fsanitize=address,undefined', '-I' + str(tmp), '-I' + str(test_dir),
                        *flags, str(test_dir / 'pioneer_qwen35_resident_host_fleet_audit_test.cpp'),
                        '-o', str(binary)], check=True)
        env = dict(os.environ, ASAN_OPTIONS='quarantine_size_mb=32', UBSAN_OPTIONS='halt_on_error=1')
        subprocess.run([str(binary)], check=True, env=env)
