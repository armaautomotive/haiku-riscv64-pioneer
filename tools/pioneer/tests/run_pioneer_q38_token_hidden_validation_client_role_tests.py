#!/usr/bin/env python3
"""Actual user header compilation gates, not execution or device access."""
import os
import pathlib
import re
import subprocess

assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
header = pathlib.Path(__file__).resolve().parents[1]/'PioneerQ38TokenHiddenValidationSession.h'
gate = header.read_text().split('#ifndef PIONEER_Q38_TOKEN_HIDDEN_VALIDATION_SESSION_H')[0]
forbidden = set(re.findall(r'defined\(([A-Z_][A-Z0-9_]*)\)', gate))-{'PIONEER_Q38_TOKEN_HIDDEN_VALIDATION_CLIENT'}
assert len(forbidden) == 14
assert {'PIONEER_Q38_TOKEN_HIDDEN_VALIDATION_NATIVE','PIONEER_Q38_TOKEN_VALIDATION_CLIENT',
        'PIONEER_Q38_TOKEN_HELD_HIDDEN_DIAGNOSTIC','PIONEER_Q38_BENCHMARK_HOST'} <= forbidden
positive = negative = 0
def check(values, expected, tail=''):
    global positive, negative
    prefix = '#define B_DEVICE_OP_CODES_END 1000\n'+''.join(f'#define {k} {v}\n' for k, v in values.items())
    result = subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-Wall', '-Wextra', '-Werror',
                             '-fsyntax-only', '-x', 'c++', '-'], input=prefix+f'#include "{header}"\n'+tail,
                            text=True, stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
    assert (result.returncode == 0) == expected, (values, result.stderr)
    if expected:
        positive += 1
    else:
        assert 'error:' in result.stderr
        assert 'Exact separate held-hidden diagnostic client required' in result.stderr or 'Held-hidden client cannot alias' in result.stderr
        negative += 1
base = {'PIONEER_Q38_TOKEN_HIDDEN_VALIDATION_CLIENT':'1'}
check(base, True)
for value in (None, '0', '2'):
    check({} if value is None else {'PIONEER_Q38_TOKEN_HIDDEN_VALIDATION_CLIENT':value}, False)
for name in sorted(forbidden):
    for value in ('0', '1'):
        check({**base, name:value}, False)
    check(base, False, f'#define {name} 0\n#include "{header}"\n')
check(base, False, f'#undef PIONEER_Q38_TOKEN_HIDDEN_VALIDATION_CLIENT\n#define PIONEER_Q38_TOKEN_HIDDEN_VALIDATION_CLIENT 0\n#include "{header}"\n')
print(f'PASS actual DEBUG75 USER client header/{positive} positive/{negative} missing+zero+two+late+kernel+native+CPU+Work+encoding refusals;syntax ONLY,not source authentication,native,physical or TPS proof')
