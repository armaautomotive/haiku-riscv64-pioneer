#!/usr/bin/env python3
"""Actual USER session/packet over synthetic source/transport; NO device IO."""
import os
from pathlib import Path
import subprocess
import tempfile

assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
here = Path(__file__).resolve().parent
with tempfile.TemporaryDirectory(prefix='pioneer-fast-decode-session-', dir='/private/tmp') as name:
    out = Path(name)
    for kind in ('protocol', 'session'):
        binary = out/kind
        subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                        '-Wno-misleading-indentation', '-ffp-contract=off', '-fsanitize=address,undefined',
                        '-DPIONEER_Q38_FULL_OFFLOAD_FAST_DECODE_PROFILE=1', '-DPIONEER_Q38_FULL_OFFLOAD_CLIENT=1',
                        '-DPIONEER_Q38_FULL_OFFLOAD_BATCHED=1', '-DPIONEER_Q38_FULL_OFFLOAD_METRICS=1',
                        str(here/('pioneer_q38_full_offload_'+kind+'_test.cpp')), '-o', str(binary)], check=True)
        subprocess.run([str(binary)], check=True, env={**os.environ, 'ASAN_OPTIONS': 'quarantine_size_mb=32', 'UBSAN_OPTIONS': 'halt_on_error=1'})
print('PASS fast decode USER protocol/session: exact8807/v3/HOLD+full-record-drain before read/END+Close before publication/all5EOS+3cancel boundaries;SYNTHETIC_NO_HARDWARE_OR_TPS', flush=True)
