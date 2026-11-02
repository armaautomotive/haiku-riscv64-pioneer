#!/usr/bin/env python3
"""Distinct corrected-ROOT native TEST syntax; no model execution/device IO."""
import os
import pathlib
import re
import runpy
import subprocess
import sys

assert len(sys.argv) == 1 and not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
here = pathlib.Path(__file__).resolve().parent
saved = sys.argv[:]
try:
    sys.argv = [str(here / 'run_pioneer_q38_frontier_failure_link_roles.py'), '--native-test']
    old = runpy.run_path(sys.argv[0], run_name='__main__')
finally:
    sys.argv = saved
sys.stdout.flush()
flags = {'PIONEER_Q38_TOKEN_BATCHED_OUTPUT_CLOSURE_PROGRAMS',
         'PIONEER_Q38_TOKEN_BATCHED_OUTPUT_CLOSURE_NATIVE_CONTEXT',
         'PIONEER_Q38_TOKEN_NATIVE_BATCHED_OUTPUT_CLOSURE_FIXTURE'}
base = {**old['base'], **dict.fromkeys(flags, '1')}
required = {key for key, value in base.items() if value == '1'}
role = here / 'pioneer_q38_token_native_batched_output_closure_role.h'
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
    assert (result.returncode == 0) == okay, (values, body, result.stderr)
    if okay:
        positive += 1
    else:
        assert 'error:' in result.stderr
        negative += 1


check(base, True)
check(base, True, include(role) + include(selected) + include(role) +
      'static_assert(PioneerQ38TokenPrograms::BatchedOutputClosure==1 && PioneerQ38TokenPrograms::GlobalOutputReadsPerGroup==10);\n')
check(old['base'], True, include(here.parent / 'PioneerQ38TokenBatchedOutputClosureProgramsRole.h') +
      'static_assert(PioneerQ38TokenBatchedOutputClosureDataSelection==0);\n')
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
foreign = old['foreign'] | set(re.findall(r'defined\(([A-Z_][A-Z0-9_]*)\)', boundary.read_text()))
foreign -= required | {own_guard}
for key in sorted(foreign):
    for value in ('0', '1'):
        check({**base, key: value}, False)
        check(base, False, include(role) + '#define ' + key + ' ' + value + '\n' + include(role))
for flag in sorted(flags):
    missing = base.copy()
    del missing[flag]
    # A production package may be selected before its FIRST TEST include.
    # Freeze/recheck the TEST marker at the TEST boundary, not an unrelated
    # production include that deliberately has no fixture authority.
    first = role if flag == 'PIONEER_Q38_TOKEN_NATIVE_BATCHED_OUTPUT_CLOSURE_FIXTURE' else selected
    check(missing, False, include(first) + '#define ' + flag + ' 1\n' + include(role))
    check(base, False, include(selected) + '#undef ' + flag + '\n' + include(role))
for value in ('0', '1'):
    check({**base, own_guard: value}, False, include(selected) + 'static_assert(PioneerQ38TokenPrograms::BatchedOutputClosure==1);\n')
for name in ('PioneerQ38TokenTwinBatchPrefetchPrograms.h', 'PioneerQ38TokenBatchedPrefetchPrograms.h'):
    check(base, False, include(here.parent / name) + include(role))
    check(base, False, include(role) + include(selected) + include(here.parent / name))
print(f'PASS corrected-output native TEST/{positive} syntax positives/{negative} missing,zero,two,late,ABI,'
      'old-array,opposite-native,HOST,USER,CPU+Work refusals;ALL unchanged QFL1 TEST+production ancestors FIRST;'
      'NOT instantiated native lifecycle,original-model arithmetic,hardware or TPS proof')
