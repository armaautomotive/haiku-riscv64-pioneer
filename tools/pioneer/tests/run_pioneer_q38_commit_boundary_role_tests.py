#!/usr/bin/env python3
"""Commit-boundary ROOT BUILD role ONLY;no execution or accelerator IO."""
import os
import pathlib
import re
import subprocess

assert not any(key.startswith(('PIONEER_', 'TT_PIONEER_')) for key in os.environ)
role = pathlib.Path(__file__).resolve().parent.parent / 'PioneerQ38TokenCommitBoundaryRole.h'
text = role.read_text()
pattern = r'!defined\(([A-Z_][A-Z0-9_]*)\)\s*\|\|\s*\1\s*!=\s*1\b'
selector = 'PIONEER_Q38_TOKEN_COMMIT_BOUNDARY_SEALS_IMAGE'
base = dict.fromkeys(set(re.findall(pattern, text)) | {selector, '__riscv'}, '1')
positive = negative = 0


def check(values, okay, tail=''):
    global positive, negative
    source = ''.join('#define ' + key + ' ' + value + '\n' for key, value in sorted(values.items()))
    source += '#include "' + str(role) + '"\n' + tail
    result = subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++11', '-Werror',
                             '-fsyntax-only', '-x', 'c++', '-'],
                            input=source, text=True, capture_output=True)
    assert (result.returncode == 0) == okay, (values, tail, result.stderr)
    if okay:
        positive += 1
    else:
        assert 'error:' in result.stderr
        negative += 1


include = '#include "' + str(role) + '"\n'
check({}, True, 'static_assert(PioneerQ38TokenCommitBoundaryProfile==0,"default");\n')
check(base, True, include + 'static_assert(PioneerQ38TokenCommitBoundaryProfile==1,"isolated");\n')
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
foreign = set(re.findall(r'defined\(([A-Z_][A-Z0-9_]*)\)', text)) - set(base)
for key in sorted(foreign):
    for value in ('0', '1'):
        check({**base, key: value}, False)
    check(base, False, '#define ' + key + ' 0\n' + include)
check({}, False, '#define ' + selector + ' 1\n' + include)
check(base, False, '#undef ' + selector + '\n' + include)
print(f'PASS commit-boundary BUILD role/{positive} syntax positives/{negative} '
      'missing+zero+two+late+native+worker+component+CPU+Work refusals;'
      'NO geometry execution,lifecycle,native,physical,model or TPS qualification')
