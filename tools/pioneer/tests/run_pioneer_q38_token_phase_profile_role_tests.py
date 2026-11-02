#!/usr/bin/env python3
"""Distinct phase timing BUILD role refusals; syntax only, no device IO."""
import os
import pathlib
import re
import subprocess

assert not any(key.startswith(('PIONEER_', 'TT_PIONEER_')) for key in os.environ)
role = pathlib.Path(__file__).resolve().parent.parent / 'PioneerQ38TokenPhaseProfileImageRole.h'
required = {'__riscv', 'PIONEER_Q38_TOKEN_PHASE_PROFILE_IMAGE', 'PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE',
            'PIONEER_Q38_GLOBAL_CHAIN_FEEDBACK', 'PIONEER_Q38_TOKEN_CANCEL_BOUNDARY',
            'PIONEER_Q38_TOKEN_FRONTIER_IMAGE', 'PIONEER_Q38_BATCHED_OUTPUT_CLOSURE_ROOT_IMAGE'}
base = dict.fromkeys(required, '1')
positive = negative = 0
include = '#include "' + str(role) + '"\n'


def check(values, okay, tail=''):
    global positive, negative
    source = ''.join('#define ' + key + ' ' + value + '\n' for key, value in sorted(values.items()))
    result = subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-Werror', '-fsyntax-only', '-x', 'c++', '-'],
                            input=source + include + tail, text=True, capture_output=True)
    assert (result.returncode == 0) == okay, (values, tail, result.stderr)
    if okay:
        positive += 1
    else:
        assert 'error:' in result.stderr
        negative += 1


check({}, True, include)
check(base, True, include)
for key in sorted(required):
    for value in (None, '0', '2'):
        bad = base.copy()
        if value is None:
            del bad[key]
        else:
            bad[key] = value
        # Entire selector absent is the legitimate unchanged ROOT binding.
        check(bad, key == 'PIONEER_Q38_TOKEN_PHASE_PROFILE_IMAGE' and value is None)
    check(base, False, '#undef ' + key + '\n#define ' + key + ' 0\n' + include)
    check(base, False, '#undef ' + key + '\n' + include)
check({}, False, '#define PIONEER_Q38_TOKEN_PHASE_PROFILE_IMAGE 1\n' + include)
foreign = set(re.findall(r'defined\(([A-Z_][A-Z0-9_]*)\)', role.read_text())) - required
for key in sorted(foreign):
    for value in ('0', '1'):
        check({**base, key: value}, False)
    check(base, False, '#define ' + key + ' 0\n' + include)
print(f'PASS phase timing BUILD role/{positive} positive/{negative} missing+zero+two+late+kernel+native+CPU+Work refusals;'
      'syntax ONLY,not runtime/model/physical/TPS qualification')
