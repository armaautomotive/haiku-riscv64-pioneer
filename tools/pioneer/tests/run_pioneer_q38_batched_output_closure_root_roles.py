#!/usr/bin/env python3
"""New ROOT role syntax/refusals; no emitted code, native owner or hardware."""
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
    sys.argv = [str(here / 'run_pioneer_q38_batched_prefetch_root_roles.py')]
    old = runpy.run_path(sys.argv[0], run_name='__main__')
finally:
    sys.argv = saved
role = here.parent / 'PioneerQ38BatchedOutputClosureRootRole.h'
include = '#include "' + str(role) + '"\n'
primary = 'PIONEER_Q38_BATCHED_OUTPUT_CLOSURE_ROOT_IMAGE'
all_flags = set(re.findall(r'defined\(([A-Z_][A-Z0-9_]*)\)', role.read_text())) | old['all_flags']
positive = negative = 0


def check(values, okay, tail='', selected=1):
    global positive, negative
    source = ''.join('#define ' + key + ' ' + value + '\n' for key, value in sorted(values.items()))
    source += include + include
    source += 'static_assert(PioneerQ38BatchedOutputClosureRootSelection==' + str(selected) + ',"expected selection");\n' + tail
    result = subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++11', '-Werror', '-fsyntax-only', '-x', 'c++', '-'],
                            input=source, text=True, capture_output=True)
    assert (result.returncode == 0) == okay, (values, tail, result.stderr)
    if okay:
        positive += 1
    else:
        assert 'error:' in result.stderr
        negative += 1


for target, other in (('PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE', 'PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE'),
                      ('PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE', 'PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE')):
    base = {**old['old']['base'], 'PIONEER_Q38_BATCHED_PREFETCH_ROOT_IMAGE': '1', primary: '1', target: '1'}
    check(base, True)
    for key in base:
        for value in (None, '0', '2'):
            bad = base.copy()
            if value is None:
                del bad[key]
            else:
                bad[key] = value
            check(bad, False)
        check(base, False, '#undef ' + key + '\n#define ' + key + ' 0\n' + include)
        check(base, False, '#undef ' + key + '\n' + include)
    for key in sorted(all_flags - set(base)):
        for value in ('0', '1'):
            check({**base, key: value}, False)
            check(base, False, '#define ' + key + ' ' + value + '\n' + include)
    for value in ('0', '1'):
        check({**base, other: value}, False)
    check({**base, '__riscv_xlen': '64'}, False)
check({}, True, selected=0)
check({}, False, '#define ' + primary + ' 1\n' + include, selected=0)
print(f'PASS batched output closure ROOT/{positive} syntax positives/{negative} missing,zero,two,late,architecture,'
      'HOST,worker,oldDATA,native,CPU+Work refusals;ALL unchanged batched ROOT ancestors FIRST;'
      'NO emitted execution,connected model,native ownership,hardware or TPS proof')
