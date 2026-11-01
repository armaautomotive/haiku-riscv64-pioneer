#!/usr/bin/env python3
"""Private/physical selector syntax only;no native lifecycle or device IO."""
import os
import pathlib
import re
import subprocess
import sys

assert len(sys.argv) == 2 and sys.argv[1] in ('--private', '--physical')
assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
physical = sys.argv[1] == '--physical'
here = pathlib.Path(__file__).resolve().parent
kernel = here.parents[2] / 'src/add-ons/kernel/drivers/misc'
subprocess.run([sys.executable, str(here / 'run_pioneer_q38_cohort_local_sources_native_roles.py'), sys.argv[1]], check=True)
pattern = r'!defined\(([A-Z_][A-Z0-9_]*)\)\s*\|\|\s*\1\s*!=\s*1\b'
new_programs = 'PIONEER_Q38_TOKEN_PACKET_IDENTITY_BOUNDARY_PROGRAMS'
new_context = 'PIONEER_Q38_TOKEN_PACKET_IDENTITY_BOUNDARY_' + ('BENCHMARK_NATIVE' if physical else 'NATIVE_CONTEXT')
abi = ('api_version', 'init_hardware', 'init_driver', 'uninit_driver', 'publish_devices', 'find_device')
if physical:
    texts = [(kernel / 'PioneerQ38TokenHiddenValidationRole.h').read_text()]
    required = set(re.findall(pattern, texts[0])) | {
        'PIONEER_Q38_TOKEN_FAULT_PROGRAMS', 'PIONEER_Q38_TOKEN_FAULT_VALIDATION_NATIVE',
        'PIONEER_Q38_TOKEN_PRECISE_BETA_PROGRAMS', 'PIONEER_Q38_TOKEN_PRECISE_BETA_VALIDATION_NATIVE',
        'PIONEER_Q38_TOKEN_POSTED_CANCEL', 'PIONEER_Q38_TOKEN_JOB_BOOT_FOLD_PROGRAMS',
        'PIONEER_Q38_TOKEN_JOB_BOOT_FOLD_VALIDATION_NATIVE', 'PIONEER_Q38_TOKEN_DECODE_BENCHMARK_NATIVE',
        'PIONEER_Q38_TOKEN_FRONTIER_PROGRAMS', 'PIONEER_Q38_TOKEN_FRONTIER_NATIVE_CONTEXT',
        'PIONEER_Q38_TOKEN_FRONTIER_BENCHMARK_NATIVE', 'PIONEER_Q38_TOKEN_RETAINED_COHORT_PROGRAMS',
        'PIONEER_Q38_TOKEN_RETAINED_COHORT_BENCHMARK_NATIVE', 'PIONEER_Q38_TOKEN_COMMIT_BOUNDARY_PROGRAMS',
        'PIONEER_Q38_TOKEN_COMMIT_BOUNDARY_BENCHMARK_NATIVE',
        'PIONEER_Q38_TOKEN_COHORT_LOCAL_SOURCES_PROGRAMS', 'PIONEER_Q38_TOKEN_COHORT_LOCAL_SOURCES_BENCHMARK_NATIVE', new_programs, new_context}
    base = {**dict.fromkeys(required, '1'), 'PIONEER_DRIVER_BUILD_TAG': '0x2026103100010075ULL'}
    foreign = {'PIONEER_Q38_TOKEN_PACKET_IDENTITY_BOUNDARY_NATIVE_CONTEXT',
               'PIONEER_Q38_TOKEN_COHORT_LOCAL_SOURCES_NATIVE_CONTEXT', 'PIONEER_Q38_COHORT_LOCAL_SOURCES_IMAGE',
               'PIONEER_Q38_TOKEN_COMMIT_BOUNDARY_NATIVE_CONTEXT', 'PIONEER_Q38_TOKEN_RETAINED_COHORT_NATIVE_CONTEXT',
               'PIONEER_Q38_TOKEN_JOB_BOOT_FOLD_NATIVE_CONTEXT', 'PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY',
               'PIONEER_Q38_TOKEN_FRONTEND_PROFILE_CLIENT', 'PIONEER_Q38_TOKEN_FRONTIER_BENCHMARK_CLIENT',
               'PIONEER_Q38_TOKEN_DECODE_BENCHMARK_CLIENT', 'PIONEER_Q38_CPU_VALIDATION',
               'PIONEER_Q38_FULL_MODEL_CPU_REFERENCE', 'PIONEER_Q38_WORK_INFERENCE', 'TT_PIONEER_Q38_WORK_MODE',
               'PIONEER_Q38_PACKET_IDENTITY_BOUNDARY_IMAGE', 'PIONEER_Q38_TOKEN_COMMIT_BOUNDARY_SEALS_IMAGE',
               'PIONEER_Q38_RETAINED_CONV_CLOSURE_IMAGE', '__FAST_MATH__', *abi}
    bad_tags = (None, '0', '0x5154334e4f445256ULL', '0x2026102100010012ULL', '0x2026103000010074ULL')
else:
    texts = [(kernel / name).read_text() for name in (
        'PioneerQ38TokenNativeRole.h', 'PioneerQ38TokenFrontierNativeRole.h',
        'PioneerQ38TokenRetainedCohortNativeRole.h', 'PioneerQ38TokenCommitBoundaryNativeRole.h',
        'PioneerQ38TokenCohortLocalSourcesNativeRole.h', 'PioneerQ38TokenPacketIdentityBoundaryNativeRole.h')]
    required = set().union(*(set(re.findall(pattern, text)) for text in texts)) | {
        'PIONEER_Q38_TOKEN_PRECISE_BETA_PROGRAMS', 'PIONEER_Q38_TOKEN_FAULT_PROGRAMS',
        'PIONEER_Q38_TOKEN_PUSHED_PROGRAMS'}
    base = {**dict.fromkeys(required, '1'), 'PIONEER_DRIVER_BUILD_TAG': '0x5154334e4f445256ULL',
            **{name: 'packet_identity_private_' + name for name in abi}}
    foreign = set(re.findall(r'defined\(([A-Z_][A-Z0-9_]*)\)', ''.join(texts))) - required - {
        'PIONEER_DRIVER_BUILD_TAG', 'PIONEER_Q38_TOKEN_FOOTPRINT_H'}
    bad_tags = (None, '0', '0x51544b4e4f445256ULL', '0x2026103100010075ULL')
role = here.parent / 'PioneerQ38TokenPacketIdentityBoundaryProgramsRole.h'
selected = here.parent / 'PioneerQ38TokenSelectedPrograms.h'
header = here.parent / 'PioneerQ38TokenPacketIdentityBoundaryPrograms.h'
positive = negative = 0


def include(path):
    return '#include "' + str(path) + '"\n'


def check(values, okay, body=None):
    global positive, negative
    source = ''.join('#define ' + key + ' ' + value + '\n' for key, value in sorted(values.items()))
    source += body if body is not None else include(role) + include(role)
    result = subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-Werror',
                             '-fsyntax-only', '-x', 'c++', '-'], input=source, text=True, capture_output=True)
    assert (result.returncode == 0) == okay, (values, body, result.stderr)
    if okay:
        positive += 1
    else:
        assert 'error:' in result.stderr
        negative += 1


kind = 3 if physical else 2
check(base, True)
check(base, True, include(selected) + include(selected) + 'static_assert('
      f'PioneerQ38TokenPacketIdentityBoundarySelection=={kind} && PioneerQ38TokenPrograms::PacketIdentityBoundary==1 '
      '&& PioneerQ38TokenPrograms::ColdSourceFromStatus==1);\n')
new_flags = {new_programs, new_context}
old = {key: value for key, value in base.items() if key not in new_flags}
check(old, True, include(selected) + 'static_assert(PioneerQ38TokenPacketIdentityBoundarySelection==0 '
      '&& PioneerQ38TokenPrograms::CohortLocalSources==1);\n')
data = {key: '1' for key in required if key.endswith('_PROGRAMS')}
check(data, True, include(header) + include(header) + 'static_assert('
      'PioneerQ38TokenPacketIdentityBoundarySelection==1 && PioneerQ38TokenPacketIdentityBoundaryPrograms::PacketIdentityBoundary==1);\n')
check(data, False, include(selected))
for key in sorted(required):
    for value in (None, '0', '2'):
        bad = base.copy()
        if value is None:
            del bad[key]
        else:
            bad[key] = value
        check(bad, False)
    check(base, False, include(role) + '#undef ' + key + '\n#define ' + key + ' 0\n' + include(role))
if not physical:
    for key in abi:
        bad = base.copy()
        del bad[key]
        check(bad, False)
        check(base, False, include(role) + '#undef ' + key + '\n' + include(role))
for value in bad_tags:
    bad = base.copy()
    if value is None:
        del bad['PIONEER_DRIVER_BUILD_TAG']
    else:
        bad['PIONEER_DRIVER_BUILD_TAG'] = value
    check(bad, False)
for key in sorted(foreign):
    for value in ('0', '1'):
        check({**base, key: value}, False)
    check(base, False, include(role) + '#define ' + key + ' 0\n' + include(role))
check(old, False, include(selected) + ''.join('#define ' + key + ' 1\n' for key in sorted(new_flags)) + include(selected))
for key in sorted(new_flags):
    check(base, False, include(selected) + '#undef ' + key + '\n' + include(selected))
check(base, False, include(here.parent / 'PioneerQ38TokenCohortLocalSourcesPrograms.h') + include(selected))
print(f'PASS packet-identity-boundary {"physical" if physical else "private"} roles/{positive} syntax positives/{negative} '
      'missing+zero+two+late+tag+ABI+foreign+CPU+Work refusals;ENTIREoldsyntaxcontrols first;'
      'NO native lifecycle,physical execution,model quality,timing or deployment grant')
