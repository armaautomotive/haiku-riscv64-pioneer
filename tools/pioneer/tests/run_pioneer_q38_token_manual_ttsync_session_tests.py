#!/usr/bin/env python3
"""Original USER session+new CLI with synthetic endpoints;NO card/model grant."""
import os
import pathlib
import subprocess
import sys
import tempfile

assert len(sys.argv) == 1 and not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
here = pathlib.Path(__file__).resolve().parent
subprocess.run([sys.executable, str(here / 'run_pioneer_q38_token_constant_q4_session_tests.py')], check=True)
with tempfile.TemporaryDirectory(prefix='pioneer-manual-ttsync-session-', dir='/private/tmp') as name:
    binary = pathlib.Path(name) / 'test'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                    '-ffp-contract=off', '-fsanitize=address,undefined',
                    str(here / 'pioneer_q38_token_manual_ttsync_session_test.cpp'), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True,
                   env={**os.environ, 'ASAN_OPTIONS': 'quarantine_size_mb=32', 'UBSAN_OPTIONS': 'halt_on_error=1'})
print('PASS_MANUAL_TTSYNC_USER_SESSION whole original controls FIRST/new CLI/same851 synthetic uploader+34records/'
      'all100 packet/transport/END/Close/clock COW cases;NO device,model quality,native or TPS grant', flush=True)
