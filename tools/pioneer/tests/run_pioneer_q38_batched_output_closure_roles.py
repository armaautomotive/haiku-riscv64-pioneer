#!/usr/bin/env python3
"""Corrected ROOT private/public syntax boundaries; no device or lifecycle."""
import os
import pathlib
import re
import runpy
import subprocess
import sys

assert sys.argv[1:] in ([], ['--public'])
assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
public = '--public' in sys.argv
here = pathlib.Path(__file__).resolve().parent
saved = sys.argv[:]
try:
    sys.argv = [str(here / 'run_pioneer_q38_frontier_failure_link_roles.py'), *(['--public'] if public else [])]
    old = runpy.run_path(sys.argv[0], run_name='__main__')
finally:
    sys.argv = saved
sys.stdout.flush()
primary = 'PIONEER_Q38_TOKEN_BATCHED_OUTPUT_CLOSURE_PROGRAMS'
context = 'PIONEER_Q38_TOKEN_BATCHED_OUTPUT_CLOSURE_' + ('BENCHMARK_NATIVE' if public else 'NATIVE_CONTEXT')
base = {**old['base'], primary: '1', context: '1'}
required = {key for key, value in base.items() if value == '1'}
role = here.parent / 'PioneerQ38TokenBatchedOutputClosureProgramsRole.h'
boundary = here.parent / 'PioneerQ38TokenBatchedOutputClosureDataBoundary.h'
selected = here.parent / 'PioneerQ38TokenSelectedPrograms.h'
own_guard = 'PIONEER_Q38_TOKEN_BATCHED_OUTPUT_CLOSURE_PROGRAMS_H'
positive = negative = 0


def include(path):
    return '#include "' + str(path) + '"\n'


def check(values, okay, body=None):
    global positive, negative
    source = ''.join('#define ' + key + ' ' + value + '\n' for key, value in sorted(values.items()))
    source += body if body is not None else include(role) + include(role)
    result = subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-Werror', '-fsyntax-only', '-x', 'c++', '-'],
                            input=source, text=True, capture_output=True)
    assert (result.returncode == 0) == okay, (public, values, body, result.stderr)
    if okay:
        positive += 1
    else:
        assert 'error:' in result.stderr
        negative += 1


wanted = 3 if public else 2
check(base, True)
check(base, True, include(selected) + include(selected) +
      'static_assert(PioneerQ38TokenBatchedOutputClosureDataSelection==' + str(wanted) +
      ' && PioneerQ38TokenPrograms::BatchedOutputClosure==1 && PioneerQ38TokenPrograms::GlobalOutputReadsPerGroup==10);\n')
check(old['base'], True, include(selected) + 'static_assert(PioneerQ38TokenBatchedOutputClosureDataSelection==0 '
      '&& PioneerQ38TokenPrograms::TwinBatchPrefetch==1);\n')
for key in sorted(required):
    for value in (None, '0', '2'):
        bad = base.copy()
        if value is None:
            del bad[key]
        else:
            bad[key] = value
        check(bad, False)
    check(base, False, include(role) + '#undef ' + key + '\n#define ' + key + ' 0\n' + include(role))
    check(base, False, include(role) + '#undef ' + key + '\n' + include(role))
for key in ('api_version', 'init_hardware', 'init_driver', 'uninit_driver', 'publish_devices', 'find_device'):
    if key in base:
        bad = base.copy()
        del bad[key]
        check(bad, False)
        check(base, False, include(role) + '#undef ' + key + '\n' + include(role))
for tag in (None, '0', '0x2026102100010012ULL', '0x2026110200010076ULL'):
    bad = base.copy()
    if tag is None:
        del bad['PIONEER_DRIVER_BUILD_TAG']
    else:
        bad['PIONEER_DRIVER_BUILD_TAG'] = tag
    check(bad, False)
foreign = old['foreign'] | set(re.findall(r'defined\(([A-Z_][A-Z0-9_]*)\)', boundary.read_text()))
foreign -= required | {own_guard, 'PIONEER_Q38_TOKEN_BATCHED_OUTPUT_CLOSURE_DATA_SELECTION_H'}
for key in sorted(foreign):
    for value in ('0', '1'):
        check({**base, key: value}, False)
        check(base, False, include(role) + '#define ' + key + ' ' + value + '\n' + include(role))
check(base, False, include(here.parent / 'PioneerQ38TokenTwinBatchPrefetchProgramsRole.h'))
for flag in (primary, context):
    missing = base.copy()
    del missing[flag]
    check(missing, False, include(selected) + '#define ' + flag + ' 1\n' + include(selected))
    check(base, False, include(selected) + '#undef ' + flag + '\n' + include(selected))
for value in ('0', '1'):
    check({**base, own_guard: value}, False, include(selected) + 'static_assert(PioneerQ38TokenPrograms::BatchedOutputClosure==1);\n')
for name in ('PioneerQ38TokenTwinBatchPrefetchPrograms.h', 'PioneerQ38TokenBatchedPrefetchPrograms.h',
             'PioneerQ38TokenPrefetchPrograms.h', 'PioneerQ38TokenFlatMatrixPrograms.h'):
    check(base, False, include(here.parent / name) + include(selected))
    check(base, False, include(selected) + include(here.parent / name))
print(f'PASS corrected-output {"PUBLIC" if public else "PRIVATE"}/{positive} syntax positives/{negative} '
      'missing,zero,two,late,tag,ABI,old-parent,old-array,opposite-native,HOST,USER,CPU+Work refusals;'
      'ALL unchanged QFL1+ancestors FIRST;NO native lifecycle,model,installation,hardware or TPS grant')
