#!/usr/bin/env python3
"""Actual USER session / MOCK source+transport; unchanged parent controls FIRST."""
import os
import pathlib
import subprocess
import tempfile

assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
here = pathlib.Path(__file__).resolve().parent
subprocess.run(['python3', str(here / 'run_pioneer_q38_frontier_failure_report_tests.py')], check=True)
with tempfile.TemporaryDirectory(prefix='pioneer-q38-phase-session-', dir='/private/tmp') as name:
    binary = pathlib.Path(name) / 'test'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                    '-ffp-contract=off', '-fsanitize=address,undefined', str(here / 'pioneer_q38_token_phase_profile_session_test.cpp'),
                    '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True, env={**os.environ, 'ASAN_OPTIONS': 'quarantine_size_mb=32', 'UBSAN_OPTIONS': 'halt_on_error=1'})
