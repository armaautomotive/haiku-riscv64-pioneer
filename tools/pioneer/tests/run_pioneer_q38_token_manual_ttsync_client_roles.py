#!/usr/bin/env python3
"""Distinct manual-completion USER perimeter;unchanged full parents FIRST."""
import os
import pathlib
import re
import runpy
import subprocess
import sys

assert len(sys.argv) == 1 and not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
here = pathlib.Path(__file__).resolve().parent
old = runpy.run_path(str(here / 'run_pioneer_q38_token_constant_q4_client_roles.py'))
primary = 'PIONEER_Q38_TOKEN_MANUAL_TTSYNC_CLIENT'
base = {**old['base'], primary: '1'}
role = here.parent / 'PioneerQ38TokenManualTTSyncClientRole.h'
cli = here.parent / 'PioneerQ38TokenManualTTSyncCli.h'
foreign = (old['foreign'] | set(re.findall(r'defined\(([A-Z_][A-Z0-9_]*)\)', role.read_text()))) - set(base)
positive = negative = 0
include = '#include "' + str(role) + '"\n'

def syntax(values, okay, body=None):
    global positive, negative
    source = ''.join('#define ' + key + ' ' + value + '\n' for key, value in sorted(values.items()))
    source += body if body is not None else include + include
    result = subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-Wall', '-Wextra', '-Werror',
                             '-fsyntax-only', '-x', 'c++', '-'], input=source, text=True, capture_output=True)
    assert (result.returncode == 0) == okay, (values, body, result.stderr)
    if okay:
        positive += 1
    else:
        assert 'error:' in result.stderr
        negative += 1

syntax({}, True)
syntax(old['base'], True)
syntax(base, True)
syntax(base, True, '#include "' + str(cli) + '"\n')
syntax(old['base'], False, '#include "' + str(cli) + '"\n')
for key in sorted(base):
    for value in (None, '', '0', '2'):
        bad = dict(base)
        if value is None:
            del bad[key]
        else:
            bad[key] = value
        # Removing the new primary alone legitimately selects legacy OFF
        # at the standalone role boundary. Its ACTUAL dedicated CLI entry
        # must reject that omission,not a test-only selection assertion.
        syntax(bad, False, '#include "' + str(cli) + '"\n')
    syntax(base, False, include + '#undef ' + key + '\n' + include)
for key in sorted(foreign):
    for value in ('0', '1'):
        syntax({**base, key: value}, False)
        syntax(base, False, include + '#define ' + key + ' ' + value + '\n' + include)
syntax(old['base'], False, include + '#define ' + primary + ' 1\n' + include)
print(f'PASS_MANUAL_TTSYNC_USER_ROLES positives{positive} productionRefusals{negative}/ALL old constant+phase+report controls FIRST;'
      'NO DATA,shader,native,CPU-reference or Work alias;NO lifecycle,model,hardware or TPS grant', flush=True)
