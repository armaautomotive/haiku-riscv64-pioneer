#!/usr/bin/env python3
"""Frontend profile USER-role containment only,NO accelerator IO."""
import os
import pathlib
import re
import subprocess

assert not any(key.startswith(('PIONEER_', 'TT_PIONEER_')) for key in os.environ)
role = pathlib.Path(__file__).resolve().parent.parent / 'PioneerQ38TokenFrontendProfileClientRole.h'
required = {'PIONEER_Q38_TOKEN_FRONTEND_PROFILE_CLIENT', 'PIONEER_Q38_TOKEN_FRONTIER_BENCHMARK_CLIENT',
            'PIONEER_Q38_TOKEN_DECODE_BENCHMARK_CLIENT', 'PIONEER_Q38_TOKEN_HIDDEN_VALIDATION_CLIENT'}
base = dict.fromkeys(required, '1')
positive = negative = 0


def check(values, okay, tail=''):
    global positive, negative
    source = ''.join('#define ' + key + ' ' + value + '\n' for key, value in sorted(values.items()))
    source += '#include "' + str(role) + '"\n' + tail
    result = subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-Werror', '-fsyntax-only', '-x', 'c++', '-'],
                            input=source, text=True, capture_output=True)
    assert (result.returncode == 0) == okay, (values, tail, result.stderr)
    if okay:
        positive += 1
    else:
        assert 'error:' in result.stderr
        negative += 1


include = '#include "' + str(role) + '"\n'
check(base, True, include)
for key in sorted(required):
    for value in (None, '0', '2'):
        bad = base.copy()
        if value is None:
            del bad[key]
        else:
            bad[key] = value
        check(bad, False)
    check(base, False, '#undef ' + key + '\n#define ' + key + ' 0\n' + include)
text = ''.join((role.parent / filename).read_text() for filename in (
    role.name, 'PioneerQ38TokenFrontierBenchmarkClientRole.h', 'PioneerQ38TokenDecodeBenchmarkClientRole.h'))
foreign = set(re.findall(r'defined\(([A-Z_][A-Z0-9_]*)\)', text)) - required
for key in sorted(foreign):
    for value in ('0', '1'):
        check({**base, key: value}, False)
    check(base, False, '#define ' + key + ' 0\n' + include)
print(f'PASS frontend USER role/{positive} positive/{negative} missing+zero+two+late+native+CPU+Work refusals;'
      'syntax ONLY,not runtime/native/model/physical/TPS qualification')
