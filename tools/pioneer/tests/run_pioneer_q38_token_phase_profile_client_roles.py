#!/usr/bin/env python3
"""Phase timing USER containment; unchanged failed-reply controls FIRST."""
import os
import pathlib
import re
import runpy
import subprocess
import sys

assert len(sys.argv) == 1 and not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
here = pathlib.Path(__file__).resolve().parent
old = runpy.run_path(str(here / 'run_pioneer_q38_frontier_failure_report_roles.py'))
primary = 'PIONEER_Q38_TOKEN_PHASE_PROFILE_CLIENT'
base = {**old['base'], primary: '1'}
role = here.parent / 'PioneerQ38TokenPhaseProfileClientRole.h'
cli = here.parent / 'PioneerQ38TokenPhaseProfileCli.h'
foreign = (old['foreign'] | set(re.findall(r'defined\(([A-Z_][A-Z0-9_]*)\)', role.read_text()))) - set(base)
positive = negative = 0
include = '#include "' + str(role) + '"\n'


def syntax(values, okay, body=None, selection=1):
    global positive, negative
    source = ''.join('#define ' + k + ' ' + v + '\n' for k, v in sorted(values.items()))
    source += body if body is not None else include + include + f'static_assert(PioneerQ38TokenPhaseProfileClientSelection=={selection});\n'
    result = subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-Wall', '-Wextra', '-Werror',
                             '-fsyntax-only', '-x', 'c++', '-'], input=source, text=True, capture_output=True)
    assert (result.returncode == 0) == okay, (values, body, result.stderr)
    if okay:
        positive += 1
    else:
        assert 'error:' in result.stderr
        negative += 1


syntax({}, True, selection=0)
syntax(old['base'], True, selection=0)
syntax(base, True)
for key in sorted(base):
    for value in (None, '0', '2'):
        bad = base.copy()
        if value is None:
            del bad[key]
        else:
            bad[key] = value
        syntax(bad, False)
    syntax(base, False, include + '#undef ' + key + '\n#define ' + key + ' 0\n' + include)
    syntax(base, False, include + '#undef ' + key + '\n' + include)
for key in sorted(foreign):
    for value in ('0', '1'):
        syntax({**base, key: value}, False)
        syntax(base, False, include + '#define ' + key + ' ' + value + '\n' + include)
syntax(old['base'], False, include + '#define ' + primary + ' 1\n' + include)
syntax(base, True, '#include "' + str(cli) + '"\n')
syntax(old['base'], False, '#include "' + str(cli) + '"\n')
print(f'PASS token-phase USER/{positive} positives/{negative} missing,zero,two,late,DATA,native,shader,CPU+Work refusals;'
      'unchanged failed-reply controls FIRST;NO native lifetime,physical,quality,Work or speed grant')
