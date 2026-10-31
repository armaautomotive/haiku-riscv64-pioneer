#!/usr/bin/env python3
"""Retained-cohort private selector containment, not execution or device IO."""
import os
import pathlib
import re
import subprocess

assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
here = pathlib.Path(__file__).resolve().parent
kernel = here.parents[2] / 'src/add-ons/kernel/drivers/misc'
texts = [(kernel / name).read_text() for name in (
    'PioneerQ38TokenNativeRole.h', 'PioneerQ38TokenFrontierNativeRole.h',
    'PioneerQ38TokenRetainedCohortNativeRole.h')]
role = here.parent / 'PioneerQ38TokenRetainedCohortProgramsRole.h'
selected = here.parent / 'PioneerQ38TokenSelectedPrograms.h'
programs = here.parent / 'PioneerQ38TokenRetainedCohortPrograms.h'
pattern = r'!defined\(([A-Z_][A-Z0-9_]*)\)\s*\|\|\s*\1\s*!=\s*1\b'
required = set().union(*(set(re.findall(pattern, text)) for text in texts)) | {
    'PIONEER_Q38_TOKEN_PRECISE_BETA_PROGRAMS', 'PIONEER_Q38_TOKEN_FAULT_PROGRAMS',
    'PIONEER_Q38_TOKEN_PUSHED_PROGRAMS'}
assert len(set(re.findall(pattern, texts[0]))) == 21
abi = ('api_version', 'init_hardware', 'init_driver', 'uninit_driver',
       'publish_devices', 'find_device')
base = {**dict.fromkeys(required, '1'),
        'PIONEER_DRIVER_BUILD_TAG': '0x5154334e4f445256ULL',
        **{name: 'retained_private_test_' + name for name in abi}}
positive = negative = 0


def include(path):
    return '#include "' + str(path) + '"\n'


def check(values, okay, body=None):
    global positive, negative
    source = ''.join('#define ' + key + ' ' + value + '\n'
                     for key, value in sorted(values.items()))
    source += body if body is not None else include(role) + include(role)
    result = subprocess.run(
        [os.environ.get('CXX', 'c++'), '-std=c++17', '-Werror',
         '-fsyntax-only', '-x', 'c++', '-'],
        input=source, text=True, capture_output=True)
    assert (result.returncode == 0) == okay, (values, body, result.stderr)
    if okay:
        positive += 1
    else:
        assert 'error:' in result.stderr
        negative += 1


check(base, True)
check(base, True, include(selected) + include(selected) +
      'static_assert(PioneerQ38TokenRetainedCohortSelection==2 && '
      'PioneerQ38TokenFrontierProgramSelection==2 && '
      'PioneerQ38TokenPrograms::RetainedConvClosure==1);\n')
new_flags = {'PIONEER_Q38_TOKEN_RETAINED_COHORT_PROGRAMS',
             'PIONEER_Q38_TOKEN_RETAINED_COHORT_NATIVE_CONTEXT'}
old = {key: value for key, value in base.items() if key not in new_flags}
check(old, True, include(selected) +
      'static_assert(PioneerQ38TokenRetainedCohortSelection==0 && '
      'PioneerQ38TokenFrontierProgramSelection==2 && '
      'PioneerQ38TokenPrograms::TokenFrontier==1);\n')
data = {key: '1' for key in required if key.endswith('_PROGRAMS')}
check(data, True, include(programs) + include(programs) +
      'static_assert(PioneerQ38TokenRetainedCohortSelection==1 && '
      'PioneerQ38TokenRetainedCohortPrograms::RetainedConvClosure==1);\n')
check(data, False, include(selected))
for key in sorted(required):
    for value in (None, '0', '2'):
        bad = base.copy()
        if value is None:
            del bad[key]
        else:
            bad[key] = value
        check(bad, False)
    check(base, False, include(role) + '#undef ' + key +
          '\n#define ' + key + ' 0\n' + include(role))
for key in abi:
    bad = base.copy()
    del bad[key]
    check(bad, False)
    check(base, False, include(role) + '#undef ' + key + '\n' + include(role))
for value in (None, '0', '0x51544b4e4f445256ULL', '0x2026103100010075ULL'):
    bad = base.copy()
    if value is None:
        del bad['PIONEER_DRIVER_BUILD_TAG']
    else:
        bad['PIONEER_DRIVER_BUILD_TAG'] = value
    check(bad, False)
foreign = set(re.findall(r'defined\(([A-Z_][A-Z0-9_]*)\)', ''.join(texts))) - required - {
    'PIONEER_DRIVER_BUILD_TAG', 'PIONEER_Q38_TOKEN_FOOTPRINT_H'}
for key in sorted(foreign):
    for value in ('0', '1'):
        check({**base, key: value}, False)
    check(base, False, include(role) + '#define ' + key + ' 0\n' + include(role))
check(old, False, include(selected) +
      ''.join('#define ' + key + ' 1\n' for key in sorted(new_flags)) + include(selected))
for key in sorted(new_flags):
    check(base, False, include(selected) + '#undef ' + key + '\n' + include(selected))
check(base, False, include(here.parent / 'PioneerQ38TokenFrontierPrograms.h') + include(selected))
print(f'PASS retained-cohort private roles/{positive} syntax positives/{negative} '
      'missing+zero+two+late+tag+ABI+physical+USER+CPU+Work refusals/'
      'old private frontier selector still old firmware;syntax ONLY,NO native lifecycle,'
      'public driver,physical timing,model quality or Work qualification')
