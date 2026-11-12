#!/usr/bin/env python3
"""Compile the current native handoff body against controlled fake endpoints."""
import os
from pathlib import Path
import subprocess
import tempfile

here = Path(__file__).resolve().parent
root = here.parents[2]
native = (root / 'src/add-ons/kernel/drivers/misc/PioneerQ38QgwdResidentNative.inc').read_text()
start = '__attribute__((noinline)) bool\nPioneerQ38TokenCallScope::NextQgwdResidentRequest'
end = '\nstatic __attribute__((noinline)) status_t Q38QgwdResidentSupport'
assert native.count(start) == 1 and native.count(end) == 1
body = start + native.split(start, 1)[1].split(end, 1)[0]
fixture = (here / 'pioneer_q38_qgwd_resident_handoff_mock.cpp').read_text()
assert fixture.count('// HANDOFF_BODY') == 1
source = fixture.replace('// HANDOFF_BODY', body)
with tempfile.TemporaryDirectory(prefix='qgwd-resident-handoff-', dir='/private/tmp') as tmp:
    binary = str(Path(tmp) / 'test')
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                    '-Wno-misleading-indentation', '-fsanitize=address,undefined', '-I', str(here),
                    '-x', 'c++', '-', '-o', binary], input=source, text=True, check=True)
    subprocess.run([binary], check=True, env=dict(os.environ, ASAN_OPTIONS='quarantine_size_mb=16',
                                                  UBSAN_OPTIONS='halt_on_error=1'))
