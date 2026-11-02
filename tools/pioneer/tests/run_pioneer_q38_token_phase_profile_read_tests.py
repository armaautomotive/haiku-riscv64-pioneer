#!/usr/bin/env python3
"""Actual held member/Fail; explicit MOCK authority, never hardware."""
import hashlib
import os
import pathlib
import subprocess
import tempfile

assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
here = pathlib.Path(__file__).resolve().parent
kernel = here.parents[2] / 'src/add-ons/kernel/drivers/misc'
source = (kernel / 'PioneerQ38TokenNativeOwner.h').read_text()


def extract(signature):
    assert source.count(signature) == 1
    first = source.index(signature)
    brace = source.index('{', first)
    depth, end = 1, brace + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[first:end]


record = extract('struct PioneerQ38TokenPhaseProfileResult {') + ';'
failure = extract('template<class IO> bool Fail(IO& io,PioneerQ38TokenPhaseProfileResult& r)')
with tempfile.TemporaryDirectory(prefix='pioneer-q38-phase-read-', dir='/private/tmp') as name:
    out = pathlib.Path(name)
    (out / 'actual_phase_result.inc').write_text(record + '\n')
    (out / 'actual_phase_fail.inc').write_text(failure + '\n')
    binary = out / 'test'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=address,undefined', '-I' + str(out), str(here / 'pioneer_q38_token_phase_profile_read_test.cpp'),
                    '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True, env={**os.environ, 'UBSAN_OPTIONS': 'halt_on_error=1'})
print('ACTUAL_PHASE_READ_PLUS_FAIL_SHA256 ' + hashlib.sha256(
    ((kernel / 'PioneerQ38TokenPhaseProfileRead.inc').read_text() + failure).encode()).hexdigest() +
    ';scope/source/clock/HOLD/frontier/quarantine are MOCK;not genuine native lifetime or hardware qualification')
