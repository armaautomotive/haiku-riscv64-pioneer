#!/usr/bin/env python3
"""CPU-only source observer separation;no compiler output/device/context."""
import os
import pathlib
import subprocess

here = pathlib.Path(__file__).resolve().parent
header = here.parent / 'PioneerQ38GdnOutputValidationReference.h'
source = header.read_text()
prefix = source.split('#ifndef PIONEER_Q38_GDN_OUTPUT_VALIDATION_REFERENCE_H', 1)[0]
clean = {k: v for k, v in os.environ.items() if not k.startswith('PIONEER_')}
cxx = os.environ.get('CXX', 'c++')
valid = {'PIONEER_Q38_GDN_OUTPUT_VALIDATION_CLIENT': '1', 'PIONEER_Q38_CPU_VALIDATION': '1'}
count = 0


def check(flags, expected, text=prefix):
    global count
    result = subprocess.run([cxx, '-x', 'c++', '-E', *['-D' + k + '=' + v for k, v in flags.items()], '-'],
                            input=text, text=True, capture_output=True, env=clean, timeout=5)
    assert (result.returncode == 0) == expected, (flags, result.stderr)
    count += 1


check(valid, True)
check({}, False)
for name in valid:
    for value in (None, '0', '2', '-1'):
        flags = dict(valid)
        if value is None:
            del flags[name]
        else:
            flags[name] = value
        check(flags, False)
# Every forbidden defined flag is rejected even when set to zero.
import re
for name in sorted(set(re.findall(r'defined\(([^)]+)\)', prefix)) - set(valid)):
    for value in ('0', '1'):
        check({**valid, name: value}, False)
include = '#include "' + str(header) + '"\n'
check(valid, True, include)
check(valid, False, include + '#define PIONEER_Q38_WORK_INFERENCE 0\n' + include)
check(valid, False, include + '#define PIONEER_Q38_GDN_OUTPUT_EXECUTION_NATIVE 0\n' + include)
assert '#include "' not in source  # no card arithmetic/transport/native helper reuse
print(f'PASS gated-output CPU observer role/mixed/defined-zero/reinclude gates{count};NO Work/native/device capability')
