#!/usr/bin/env python3
"""Actual USER session with MOCK endpoints; old full controls first."""
import os
import pathlib
import subprocess
import tempfile

assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
here=pathlib.Path(__file__).resolve().parent
subprocess.run(['python3',str(here/'run_pioneer_q38_token_phase_profile_session_tests.py')],check=True)
with tempfile.TemporaryDirectory(prefix='pioneer-constant-q4-session-',dir='/private/tmp') as tmp:
    binary=pathlib.Path(tmp)/'test'
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror',
                    '-ffp-contract=off','-fsanitize=address,undefined',
                    str(here/'pioneer_q38_token_constant_q4_session_test.cpp'),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True,
                   env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
print('PASS constant-Q4 USER full original synthetic851 upload/session/held packet/Finish/Close and distinct CLI;NO device or model-quality proof')
