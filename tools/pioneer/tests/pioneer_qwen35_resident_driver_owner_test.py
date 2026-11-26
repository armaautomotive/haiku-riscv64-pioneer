#!/usr/bin/env python3
"""Compile the real resident driver owner with a simulated Haiku context."""
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
    fused = ['-DPIONEER_QWEN35_SHARED_ROLE_GRAPH_WITNESS', '-DPIONEER_QWEN35_PAIRED_RECURRENCE',
             '-DPIONEER_QWEN35_FUSED_RECURRENCE', '-DPIONEER_QWEN35_DELTA_SHARD_COMPILE_ONLY',
             '-DPIONEER_QWEN35_FLASH_DECODE_COMPILE_ONLY', '-DPIONEER_QWEN35_ACTIVATION_SHARING_WITNESS',
             '-DPIONEER_QWEN35_FFN_PAIR_COMPILE_ONLY', '-DPIONEER_QWEN35_FUSED_RMS_SAVE_COMPILE_ONLY']
    for name, flags in (('ordinary', []), ('paired', ['-DPIONEER_QWEN35_PAIRED_RECURRENCE']),
                        ('tree-paired', ['-DPIONEER_QWEN35_TREE_SETTLEMENT', '-DPIONEER_QWEN35_PAIRED_RECURRENCE']),
                        ('fused', fused), ('tree-fused', fused + ['-DPIONEER_QWEN35_TREE_SETTLEMENT'])):
        binary = tmp / name
        defines = ['-DPIONEER_QWEN35_DEVICE_DISPATCH', '-DPIONEER_QWEN35_RECOVERY_COMPILE_ONLY',
                   '-DPIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS', '-DPIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS',
                   '-DPIONEER_QWEN35_RESIDENT_SERVICE_WITNESS']
        subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra',
                        '-Werror', '-fsanitize=address,undefined', '-I' + str(tmp), '-I' + str(test_dir),
                        *defines, *flags, str(test_dir / 'pioneer_qwen35_resident_driver_owner_test.cpp'),
                        '-o', str(binary)], check=True)
        env = dict(os.environ, ASAN_OPTIONS='quarantine_size_mb=32', UBSAN_OPTIONS='halt_on_error=1')
        subprocess.run([str(binary)], check=True, env=env)
