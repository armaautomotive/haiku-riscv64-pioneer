#!/usr/bin/env python3
"""Test resident session with the real host transaction and simulated panel."""
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
    hybrid = ['-DPIONEER_QWEN35_PAIRED_RECURRENCE', '-DPIONEER_QWEN35_FUSED_RECURRENCE',
              '-DPIONEER_QWEN35_DELTA_SHARD_COMPILE_ONLY', '-DPIONEER_QWEN35_FLASH_DECODE_COMPILE_ONLY',
              '-DPIONEER_QWEN35_ACTIVATION_SHARING_WITNESS']
    for name, flags in (('ordinary', []), ('paired', ['-DPIONEER_QWEN35_PAIRED_RECURRENCE']),
                        ('hybrid', hybrid),
                        ('fused', hybrid + ['-DPIONEER_QWEN35_FFN_PAIR_COMPILE_ONLY',
                                            '-DPIONEER_QWEN35_FUSED_RMS_SAVE_COMPILE_ONLY'])):
        binary = tmp / name
        subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra',
                        '-Werror', '-fsanitize=address,undefined', '-I' + str(tmp), '-I' + str(test_dir),
                        *flags, str(test_dir / 'pioneer_qwen35_resident_graph_session_test.cpp'),
                        '-o', str(binary)], check=True)
        env = dict(os.environ, ASAN_OPTIONS='quarantine_size_mb=32', UBSAN_OPTIONS='halt_on_error=1')
        subprocess.run([str(binary)], check=True, env=env)
