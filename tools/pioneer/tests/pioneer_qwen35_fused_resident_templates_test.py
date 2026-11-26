#!/usr/bin/env python3
"""Exercise real held-template startup with the fused resident graph, using fake MMIO."""
import os
from pathlib import Path
import subprocess
import tempfile

test_dir = Path(__file__).resolve().parent
flags = ['-DPIONEER_QWEN35_RECOVERY_COMPILE_ONLY', '-DPIONEER_QWEN35_SHARED_ROLE_GRAPH_WITNESS',
         '-DPIONEER_QWEN35_PAIRED_RECURRENCE', '-DPIONEER_QWEN35_FUSED_RECURRENCE',
         '-DPIONEER_QWEN35_DELTA_SHARD_COMPILE_ONLY', '-DPIONEER_QWEN35_FLASH_DECODE_COMPILE_ONLY',
         '-DPIONEER_QWEN35_ACTIVATION_SHARING_WITNESS', '-DPIONEER_QWEN35_FFN_PAIR_COMPILE_ONLY',
         '-DPIONEER_QWEN35_FUSED_RMS_SAVE_COMPILE_ONLY', '-DPIONEER_QWEN35_TREE_SETTLEMENT']
env = dict(os.environ, ASAN_OPTIONS='quarantine_size_mb=32', UBSAN_OPTIONS='halt_on_error=1')
with tempfile.TemporaryDirectory() as temporary:
    for name in ('installer', 'service_integration'):
        binary = Path(temporary) / name
        subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra',
                        '-Werror', '-fsanitize=address,undefined', *flags,
                        str(test_dir / f'pioneer_qwen35_resident_template_{name}_test.cpp'),
                        '-o', str(binary)], check=True)
        subprocess.run([str(binary)], check=True, env=env)
