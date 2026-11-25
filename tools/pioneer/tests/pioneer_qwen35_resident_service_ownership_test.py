#!/usr/bin/env python3
"""Exercise the actual service owner with a simulated resident token session."""
import os
from pathlib import Path
import subprocess
import tempfile

test_dir = Path(__file__).resolve().parent
source = (test_dir / 'pioneer_qwen35_resident_host_pci_test.cpp').read_text()
marker = '\nint main() {'
assert source.count(marker) == 1
with tempfile.TemporaryDirectory() as temporary:
    tmp = Path(temporary)
    (tmp / 'resident-host-fixture.inc').write_text(source.split(marker, 1)[0] + '\n')
    for name, flags in (('ordinary', []), ('paired', ['-DPIONEER_QWEN35_PAIRED_RECURRENCE'])):
        binary = tmp / name
        defines = ['-DPIONEER_QWEN35_DEVICE_DISPATCH', '-DPIONEER_QWEN35_RECOVERY_COMPILE_ONLY',
                   '-DPIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS', '-DPIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS',
                   '-DPIONEER_QWEN35_RESIDENT_SERVICE_WITNESS']
        subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra',
                        '-Werror', '-fsanitize=address,undefined', '-I' + str(tmp), '-I' + str(test_dir),
                        *defines, *flags, str(test_dir / 'pioneer_qwen35_resident_service_ownership_test.cpp'),
                        '-o', str(binary)], check=True)
        env = dict(os.environ, ASAN_OPTIONS='quarantine_size_mb=32', UBSAN_OPTIONS='halt_on_error=1')
        subprocess.run([str(binary)], check=True, env=env)
