#!/usr/bin/env python3
"""Distinct commit-boundary diagnostic roles; NO accelerator/device IO."""
import os
import pathlib
import re
import subprocess
import sys

assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
here = pathlib.Path(__file__).resolve().parent
kernel = here.parents[2] / 'src/add-ons/kernel/drivers/misc'
subprocess.run([sys.executable, str(here / 'run_pioneer_q38_retained_cohort_benchmark_role_tests.py')], check=True)
parent = (kernel / 'PioneerQ38TokenHiddenValidationRole.h').read_text()
pattern = r'!defined\(([A-Z_][A-Z0-9_]*)\)\s*\|\|\s*\1\s*!=\s*1\b'
required = set(re.findall(pattern, parent)) | {
    'PIONEER_Q38_TOKEN_FAULT_PROGRAMS', 'PIONEER_Q38_TOKEN_FAULT_VALIDATION_NATIVE',
    'PIONEER_Q38_TOKEN_PRECISE_BETA_PROGRAMS', 'PIONEER_Q38_TOKEN_PRECISE_BETA_VALIDATION_NATIVE',
    'PIONEER_Q38_TOKEN_POSTED_CANCEL', 'PIONEER_Q38_TOKEN_JOB_BOOT_FOLD_PROGRAMS',
    'PIONEER_Q38_TOKEN_JOB_BOOT_FOLD_VALIDATION_NATIVE', 'PIONEER_Q38_TOKEN_DECODE_BENCHMARK_NATIVE',
    'PIONEER_Q38_TOKEN_FRONTIER_PROGRAMS', 'PIONEER_Q38_TOKEN_FRONTIER_NATIVE_CONTEXT',
    'PIONEER_Q38_TOKEN_FRONTIER_BENCHMARK_NATIVE', 'PIONEER_Q38_TOKEN_RETAINED_COHORT_PROGRAMS',
    'PIONEER_Q38_TOKEN_RETAINED_COHORT_BENCHMARK_NATIVE',
    'PIONEER_Q38_TOKEN_COMMIT_BOUNDARY_PROGRAMS', 'PIONEER_Q38_TOKEN_COMMIT_BOUNDARY_BENCHMARK_NATIVE'}
role = here.parent / 'PioneerQ38TokenCommitBoundaryProgramsRole.h'
selected = here.parent / 'PioneerQ38TokenSelectedPrograms.h'
physical = kernel / 'PioneerQ38TokenCommitBoundaryBenchmarkRole.h'
base = {**dict.fromkeys(required, '1'), 'PIONEER_DRIVER_BUILD_TAG': '0x2026103100010075ULL'}
positive = negative = 0


def include(path):
    return '#include "' + str(path) + '"\n'


def check(values, okay, body=None):
    global positive, negative
    source = ''.join('#define ' + key + ' ' + value + '\n' for key, value in sorted(values.items()))
    source += body if body is not None else include(role) + include(role)
    result = subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-Werror', '-fsyntax-only',
                             '-x', 'c++', '-'], input=source, text=True, capture_output=True)
    assert (result.returncode == 0) == okay, (values, body, result.stderr)
    if okay:
        positive += 1
    else:
        assert 'error:' in result.stderr
        negative += 1


check(base, True)
check(base, True, include(physical) + include(physical))
check(base, True, include(selected) + include(selected) + 'static_assert('
      'PioneerQ38TokenCommitBoundarySelection==3 && PioneerQ38TokenCommitBoundaryPhysicalSelection==1 && '
      'PioneerQ38TokenPrograms::CommitBoundarySeals==1);\n')
new_flags = {'PIONEER_Q38_TOKEN_COMMIT_BOUNDARY_PROGRAMS', 'PIONEER_Q38_TOKEN_COMMIT_BOUNDARY_BENCHMARK_NATIVE'}
old = {key: value for key, value in base.items() if key not in new_flags}
check(old, True, include(selected) + 'static_assert(PioneerQ38TokenCommitBoundarySelection==0 && '
      'PioneerQ38TokenPrograms::RetainedConvClosure==1);\n')
for key in sorted(required):
    for value in (None, '0', '2'):
        bad = base.copy()
        if value is None:
            del bad[key]
        else:
            bad[key] = value
        check(bad, False)
    check(base, False, include(role) + '#undef ' + key + '\n#define ' + key + ' 0\n' + include(role))
for key in ('PIONEER_Q38_TOKEN_COMMIT_BOUNDARY_NATIVE_CONTEXT', 'PIONEER_Q38_TOKEN_RETAINED_COHORT_NATIVE_CONTEXT',
            'PIONEER_Q38_TOKEN_JOB_BOOT_FOLD_NATIVE_CONTEXT', 'PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY',
            'PIONEER_Q38_TOKEN_FRONTEND_PROFILE_CLIENT', 'PIONEER_Q38_TOKEN_FRONTIER_BENCHMARK_CLIENT',
            'PIONEER_Q38_TOKEN_DECODE_BENCHMARK_CLIENT', 'PIONEER_Q38_CPU_VALIDATION',
            'PIONEER_Q38_FULL_MODEL_CPU_REFERENCE', 'PIONEER_Q38_WORK_INFERENCE', 'TT_PIONEER_Q38_WORK_MODE',
            'PIONEER_Q38_TOKEN_COMMIT_BOUNDARY_SEALS_IMAGE', 'PIONEER_Q38_RETAINED_CONV_CLOSURE_IMAGE', '__FAST_MATH__',
            'api_version', 'init_hardware', 'init_driver', 'uninit_driver', 'publish_devices', 'find_device'):
    for value in ('0', '1'):
        check({**base, key: value}, False)
    check(base, False, include(role) + '#define ' + key + ' 0\n' + include(role))
for tag in (None, '0', '0x5154334e4f445256ULL', '0x2026102100010012ULL', '0x2026103000010074ULL'):
    bad = base.copy()
    if tag is None:
        del bad['PIONEER_DRIVER_BUILD_TAG']
    else:
        bad['PIONEER_DRIVER_BUILD_TAG'] = tag
    check(bad, False)
check(old, False, include(selected) + ''.join('#define ' + key + ' 1\n' for key in sorted(new_flags)) + include(selected))
for key in sorted(new_flags):
    check(base, False, include(selected) + '#undef ' + key + '\n' + include(selected))
print('PASS commit-boundary physical roles/' + str(positive) + ' syntax positives/' + str(negative)
      + ' missing+zero+two+late+NODRV+ABI+CPU+Work refusals/old physical selector still old firmware;'
        'syntax ONLY,NO lifecycle,physical execution,model quality,timing or deployment grant')
