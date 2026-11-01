#!/usr/bin/env python3
"""Distinct batched diagnostic syntax/refusals ONLY;NO execution or deployment."""
import os
import pathlib
import re
import runpy
import subprocess
import sys

assert len(sys.argv) == 1 and not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
here = pathlib.Path(__file__).resolve().parent
kernel = here.parents[2] / 'src/add-ons/kernel/drivers/misc'
saved = sys.argv[:]
try:
    sys.argv = [str(here / 'run_pioneer_q38_token_prefetch_native_roles.py'), '--physical']
    old = runpy.run_path(sys.argv[0], run_name='__main__')
finally:
    sys.argv = saved
new_flags = {'PIONEER_Q38_TOKEN_BATCHED_PREFETCH_PROGRAMS', 'PIONEER_Q38_TOKEN_BATCHED_PREFETCH_BENCHMARK_NATIVE'}
base = {**old['base'], **dict.fromkeys(new_flags, '1')}
role = here.parent / 'PioneerQ38TokenBatchedPrefetchProgramsRole.h'
selected = here.parent / 'PioneerQ38TokenSelectedPrograms.h'
benchmark = kernel / 'PioneerQ38TokenBatchedPrefetchBenchmarkRole.h'
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
check(base, True, include(selected) + include(selected) +
      'static_assert(PioneerQ38TokenBatchedPrefetchDataSelection==3 '
      '&& PioneerQ38TokenBatchedPrefetchPhysicalSelection==1 '
      '&& PioneerQ38TokenPrograms::BatchedPrefetch==1 && PioneerQ38TokenPrograms::Q8Batch==4);\n')
check(old['base'], True, include(selected) + 'static_assert(PioneerQ38TokenBatchedPrefetchDataSelection==0 '
      '&& PioneerQ38TokenPrograms::TilePrefetch==1);\n')
required = old['required'] | new_flags
for key in sorted(required):
    for value in (None, '0', '2'):
        bad = base.copy()
        if value is None:
            del bad[key]
        else:
            bad[key] = value
        check(bad, False)
    check(base, False, include(role) + '#undef ' + key + '\n#define ' + key + ' 0\n' + include(role))
for value in (*old['bad_tags'], '0x2026110100010076ULL'):
    bad = base.copy()
    if value is None:
        del bad['PIONEER_DRIVER_BUILD_TAG']
    else:
        bad['PIONEER_DRIVER_BUILD_TAG'] = value
    check(bad, False)
foreign = old['foreign'] | (set(re.findall(r'defined\(([A-Z_][A-Z0-9_]*)\)', benchmark.read_text())) - required)
for key in sorted(foreign):
    for value in ('0', '1'):
        check({**base, key: value}, False)
    check(base, False, include(role) + '#define ' + key + ' 0\n' + include(role))
check(old['base'], False, include(selected) + ''.join('#define ' + key + ' 1\n' for key in sorted(new_flags)) + include(selected))
for key in sorted(new_flags):
    check(base, False, include(selected) + '#undef ' + key + '\n' + include(selected))
for name in ('PioneerQ38TokenPrefetchPrograms.h', 'PioneerQ38TokenFlatMatrixPrograms.h', 'PioneerQ38TokenPushedPrograms.h'):
    legacy = here.parent / name
    check(base, False, include(legacy) + include(selected))
    check(base, False, include(selected) + include(legacy))
# This OLD parent cannot select a new code family without the new outer
# role establishing its immutable binding first,even with all flags set.
check(base, False, include(here.parent / 'PioneerQ38TokenPrefetchProgramsRole.h'))
check(base, False, include(benchmark))
print(f'PASS batched bounded diagnostic/{positive} syntax positives/{negative} '
      'missing,zero,two,late,tag,ABI,oldarrays,shader,private,CPU+Work refusals;'
      'ALL unchanged prefetch+native ancestors FIRST;NO native lifecycle,installation,model,hardware or TPS grant')
