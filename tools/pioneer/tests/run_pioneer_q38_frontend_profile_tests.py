#!/usr/bin/env python3
"""USER snapshot copies and bounded samples with MOCK transport,NO device IO."""
import os
import pathlib
import subprocess
import tempfile

assert not any(key.startswith(('PIONEER_', 'TT_PIONEER_')) for key in os.environ)
here = pathlib.Path(__file__).resolve().parent
with tempfile.TemporaryDirectory(prefix='pioneer-q38-frontend-profile-', dir='/private/tmp') as name:
    out = pathlib.Path(name)
    for source in ('pioneer_q38_frontend_samples_test.cpp', 'pioneer_q38_frontend_profile_session_test.cpp'):
        binary = out / pathlib.Path(source).stem
        subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                        '-ffp-contract=off', '-fsanitize=address,undefined', str(here / source), '-o', str(binary)], check=True)
        subprocess.run([str(binary)], check=True,
                       env={**os.environ, 'ASAN_OPTIONS': 'quarantine_size_mb=32', 'UBSAN_OPTIONS': 'halt_on_error=1'})
print('PASS frontend profile USER helpers;SYNTHETIC source+OS+transport,NOT native/model/physical/timing/Work authority')
