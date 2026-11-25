#!/usr/bin/env python3
"""Exercise the resident heap owner with actual session and service code."""
import os
from pathlib import Path
import subprocess
import tempfile

test_dir = Path(__file__).resolve().parent
marker = '\nint main() {'
host = (test_dir / 'pioneer_qwen35_resident_host_pci_test.cpp').read_text()
service = (test_dir / 'pioneer_qwen35_resident_service_ownership_test.cpp').read_text()
assert host.count(marker) == service.count(marker) == 1
with tempfile.TemporaryDirectory() as temporary:
    tmp = Path(temporary)
    (tmp / 'resident-host-fixture.inc').write_text(host.split(marker, 1)[0] + '\n')
    (tmp / 'resident-service-fixture.inc').write_text(service.split(marker, 1)[0] + '\n')
    for name, flags in (('ordinary', []), ('paired', ['-DPIONEER_QWEN35_PAIRED_RECURRENCE'])):
        binary = tmp / name
        defines = ['-DPIONEER_QWEN35_DEVICE_DISPATCH', '-DPIONEER_QWEN35_RECOVERY_COMPILE_ONLY',
                   '-DPIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS', '-DPIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS',
                   '-DPIONEER_QWEN35_RESIDENT_SERVICE_WITNESS']
        subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra',
                        '-Werror', '-fsanitize=address,undefined', '-I' + str(tmp), '-I' + str(test_dir),
                        *defines, *flags, str(test_dir / 'pioneer_qwen35_resident_owned_runner_test.cpp'),
                        '-o', str(binary)], check=True)
        env = dict(os.environ, ASAN_OPTIONS='quarantine_size_mb=32', UBSAN_OPTIONS='halt_on_error=1')
        subprocess.run([str(binary)], check=True, env=env)
