#!/usr/bin/env python3
"""Shader selector syntax only;no native owner,model or device IO."""
import os
import pathlib
import re
import subprocess
import sys

assert len(sys.argv) == 1
assert not any(key.startswith(('PIONEER_', 'TT_PIONEER_')) for key in os.environ)
here = pathlib.Path(__file__).resolve().parent
role = here.parent / 'PioneerQ38TokenPrefetchWorkerRole.h'
text = role.read_text()
required = set(re.findall(r'!defined\(([A-Z_][A-Z0-9_]*)\)\s*\|\|\s*\1\s*!=\s*1\b', text))
primary = 'PIONEER_Q38_TOKEN_PREFETCH_WORKER_IMAGE'
required.add(primary)
assert len(required) == 10, required
base = dict.fromkeys(required, '1')
foreign = set(re.findall(r'defined\(([A-Z_][A-Z0-9_]*)\)', text)) - required
positive = negative = 0


def include():
    return '#include "' + str(role) + '"\n'


def check(values, okay, body=None):
    global positive, negative
    source = ''.join('#define ' + key + ' ' + value + '\n' for key, value in sorted(values.items()))
    source += body if body is not None else include() + include()
    result = subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-Wall', '-Wextra', '-Werror',
                             '-fsyntax-only', '-x', 'c++', '-'], input=source, text=True, capture_output=True)
    assert (result.returncode == 0) == okay, (values, body, result.stderr)
    if okay:
        positive += 1
    else:
        assert 'error:' in result.stderr
        negative += 1


check({}, True, include() + 'static_assert(PioneerQ38TokenPrefetchWorkerSelection==0);\n')
check(base, True, include() + include() + 'static_assert(PioneerQ38TokenPrefetchWorkerSelection==1);\n')
check({'PIONEER_Q38_WORK_TILE_PREFETCH': '1'}, True,
      include() + 'static_assert(PioneerQ38TokenPrefetchWorkerSelection==0);\n')
for key in sorted(required):
    for value in (None, '0', '2'):
        bad = base.copy()
        if value is None:
            del bad[key]
        else:
            bad[key] = value
        check(bad, False)
    check(base, False, include() + '#undef ' + key + '\n#define ' + key + ' 0\n' + include())
for key in sorted(foreign):
    for value in ('0', '1'):
        check({**base, key: value}, False)
    check(base, False, include() + '#define ' + key + ' 0\n' + include())
check({}, False, include() + ''.join('#define ' + key + ' 1\n' for key in sorted(required)) + include())
check(base, False, include() + '#undef ' + primary + '\n' + include())
check({'PIONEER_Q38_WORK_TILE_PREFETCH': '1', 'PIONEER_Q38_PROJECTION_PERSISTENT': '1'}, False)
print(f'PASS distinct full-model prefetch shader roles/{positive} syntax positives/{negative} missing+zero+two+late+native+ROOT+oldimage+CPU+Work refusals;'
      'legacy nonprojection prefetch remains distinct;NO firmware execution,numeric,lifetime,device or speed qualification')
