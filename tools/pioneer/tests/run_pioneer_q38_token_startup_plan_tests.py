#!/usr/bin/env python3
"""Full startup DATA/coverage, not native ownership or hardware execution."""
import os
import pathlib
import subprocess
import sys
import tempfile
here = pathlib.Path(__file__).resolve().parent
assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
options = sys.argv[1:]
assert len(set(options)) == len(options) and set(options) <= {'--bindings-only', '--precise-activation'}
precise = '--precise-activation' in options
with tempfile.TemporaryDirectory(prefix='pioneer-token-startup-plan-', dir='/private/tmp') as tmp:
    binary = pathlib.Path(tmp)/'test'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=address,undefined', *(['-DPIONEER_Q38_GLOBAL_CHAIN_PRECISE_ACTIVATION=1'] if precise else []),
                    str(here/'pioneer_q38_token_startup_plan_test.cpp'), '-o', str(binary)], check=True)
    subprocess.run([str(binary), *(['--bindings-only'] if '--bindings-only' in options else [])], check=True,
                   env={**os.environ, 'ASAN_OPTIONS':'quarantine_size_mb=32', 'UBSAN_OPTIONS':'halt_on_error=1'})
