#!/usr/bin/env python3
"""Isolated BUILD containment only;no instruction/model/native/device IO."""
import os
import pathlib
import re
import subprocess

assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
role = pathlib.Path(__file__).resolve().parent.parent / 'PioneerQ38TidBoundaryRole.h'
source = role.read_text()
selector = 'PIONEER_Q38_TID_BOUNDARY_IMAGE'
required = set(re.findall(r'!defined\(([A-Z_][A-Z0-9_]*)\)\s*\|\|\s*\1\s*!=\s*1\b', source))
base = dict.fromkeys(required | {selector, '__riscv'}, '1')
positive = negative = 0


def check(values, okay, tail=''):
    global positive, negative
    text = ''.join('#define ' + key + ' ' + value + '\n' for key, value in sorted(values.items()))
    text += '#include "' + str(role) + '"\n' + tail
    result = subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++11', '-Werror',
                             '-fsyntax-only', '-x', 'c++', '-'], input=text, text=True, capture_output=True)
    assert (result.returncode == 0) == okay, (values, tail, result.stderr)
    if okay:
        positive += 1
    else:
        assert 'error:' in result.stderr
        negative += 1


include = '#include "' + str(role) + '"\n'
check({}, True, 'static_assert(PioneerQ38TidBoundaryProfile==0,"default");\n')
check(base, True, include + 'static_assert(PioneerQ38TidBoundaryProfile==1,"isolated");\n')
for key in sorted(base):
    for value in (None, '0', '2'):
        bad = base.copy()
        if value is None:
            del bad[key]
        else:
            bad[key] = value
        if key == selector and value is None:
            continue
        check(bad, False)
    check(base, False, '#undef ' + key + '\n#define ' + key + ' 0\n' + include)
foreign = set(re.findall(r'defined\(([A-Z_][A-Z0-9_]*)\)', source)) - set(base)
for key in sorted(foreign):
    for value in ('0', '1'):
        check({**base, key: value}, False)
    check(base, False, '#define ' + key + ' 0\n' + include)
check({}, False, '#define ' + selector + ' 1\n' + include)
check(base, False, '#undef ' + selector + '\n' + include)
print(f'PASS tid-boundary BUILD role/{positive} syntax positives/{negative} '
      'missing+zero+two+late+native+worker+dense+DATA+CPU+Work refusals;'
      'NO instruction,model,lifecycle,physical or TPS qualification')
