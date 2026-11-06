#!/usr/bin/env python3
"""Actual phase adapter+packet with explicitly MOCK FD/OS/read authority."""
import hashlib
import os
import pathlib
import subprocess
import sys
import tempfile

assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
assert sys.argv[1:] in ([],['--bf16-profile'])
flags=['-DPIONEER_Q38_BF16_FULL_PROFILE=1'] if sys.argv[1:] else []
here = pathlib.Path(__file__).resolve().parent
kernel = here.parents[2] / 'src/add-ons/kernel/drivers/misc'


def extract(source, signature):
    assert source.count(signature) == 1
    first = source.index(signature)
    brace = source.index('{', first)
    depth, end = 1, brace + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[first:end]


record = extract((kernel / 'PioneerQ38TokenNativeOwner.h').read_text(), 'struct PioneerQ38TokenPhaseProfileResult {') + ';'
adapter = extract((kernel / 'PioneerQ38TokenPhaseProfileControl.inc').read_text(),
                  '[[maybe_unused]] static __attribute__((noinline)) status_t\nQ38TokenPhaseProfileControl(')
with tempfile.TemporaryDirectory(prefix='pioneer-q38-phase-control-', dir='/private/tmp') as name:
    out = pathlib.Path(name)
    (out / 'actual_phase_result.inc').write_text(record + '\n')
    (out / 'actual_phase_control.inc').write_text(adapter + '\n')
    binary = out / 'test'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=address,undefined', *flags, '-I' + str(out), str(here / 'pioneer_q38_token_phase_profile_control_test.cpp'),
                    '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True, env={**os.environ, 'UBSAN_OPTIONS': 'halt_on_error=1'})
print('ACTUAL_PHASE_ADAPTER_SHA256 ' + hashlib.sha256(adapter.encode()).hexdigest() +
    ';FD/source/OS/reader/retention are MOCK;not a real native owner or physical qualification')
