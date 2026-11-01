#!/usr/bin/env python3
"""Exact complete immutable DATA and refusals;no model/native/device IO."""
import hashlib
import os
import pathlib
import re
import runpy
import subprocess
import sys
import tempfile

assert len(sys.argv) == 7 and not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
root, worker, cohort, commit, retained, frontier = map(pathlib.Path, sys.argv[1:])
assert all(p.is_absolute() and not p.is_symlink() for p in (root, worker, cohort, commit, retained, frontier))
here = pathlib.Path(__file__).resolve().parent
generator = here.parent / 'generate_qwen38_token_programs.py'
helpers = runpy.run_path(str(generator))
saved = sys.argv[:]
try:
    # ENTIRE old commit-boundary/retained/frontier DATA and refusal controls
    # still pass FIRST;their headers,pins and numeric gates are not updated.
    sys.argv = [str(here / 'run_pioneer_q38_cohort_local_sources_program_tests.py'),
                str(cohort), str(worker), str(commit), str(retained), str(frontier)]
    runpy.run_path(sys.argv[0], run_name='__main__')
finally:
    sys.argv = saved
inputs = ((root, helpers['pins'](here.parent / 'build_qwen38_packet_identity_boundary_root.py', 'PINS')),
          (worker, helpers['pins'](here.parent / 'build_qwen38_precise_beta_worker_image.py', 'PINS')))
for directory, records in inputs:
    helpers['authenticated'](directory, records)
committed = here.parent / 'PioneerQ38TokenPacketIdentityBoundaryPrograms.h'
assert hashlib.sha256(committed.read_bytes()).hexdigest() == 'de0ae6307f235640a067360de2b3acf5a0e30c7de00b06c27322d4d90c1e3243'
selectors = ('PIONEER_Q38_TOKEN_PACKET_IDENTITY_BOUNDARY_PROGRAMS', 'PIONEER_Q38_TOKEN_COHORT_LOCAL_SOURCES_PROGRAMS', 'PIONEER_Q38_TOKEN_COMMIT_BOUNDARY_PROGRAMS',
             'PIONEER_Q38_TOKEN_RETAINED_COHORT_PROGRAMS', 'PIONEER_Q38_TOKEN_FRONTIER_PROGRAMS',
             'PIONEER_Q38_TOKEN_JOB_BOOT_FOLD_PROGRAMS', 'PIONEER_Q38_TOKEN_PRECISE_BETA_PROGRAMS',
             'PIONEER_Q38_TOKEN_FAULT_PROGRAMS', 'PIONEER_Q38_TOKEN_PUSHED_PROGRAMS')
with tempfile.TemporaryDirectory(prefix='pioneer-q38-packet-identity-data-', dir='/private/tmp') as temporary:
    out = pathlib.Path(temporary)
    header = out / 'new-data.h'
    subprocess.run([sys.executable, str(generator), str(root), str(worker), str(header), '--packet-identity-boundary'], check=True)
    assert header.read_bytes() == committed.read_bytes()
    common = [os.environ.get('CXX', 'c++'), '-std=c++17', '-Wall', '-Wextra', '-Werror', '-I' + str(here.parent)]
    binary = out / 'test'
    subprocess.run([*common, '-O1', '-g', '-fsanitize=address,undefined', *['-D' + k + '=1' for k in selectors],
                    '-DPIONEER_Q38_PROGRAM_DATA_TEST_PACKET_IDENTITY_BOUNDARY=1',
                    '-DPIONEER_Q38_PUSHED_PROGRAMS_HEADER="' + str(header) + '"',
                    str(here / 'pioneer_q38_pushed_programs_test.cpp'), '-o', str(binary)], check=True)
    subprocess.run([str(binary), str(root), str(worker)], check=True,
                   env={**os.environ, 'ASAN_OPTIONS': 'quarantine_size_mb=32', 'UBSAN_OPTIONS': 'halt_on_error=1'})
    corruptions = 0
    for directory, records in inputs:
        for name, _, _ in records:
            case = out / ('bad-' + str(corruptions))
            case.mkdir()
            for path in directory.iterdir():
                if path.is_file():
                    (case / path.name).symlink_to(path)
            target = case / name
            target.unlink()  # Only this fixture's freshly created exact symlink.
            original = (directory / name).read_bytes()
            damaged = bytearray(original)
            damaged[len(damaged) // 2] ^= 1
            for data in (damaged, original[:-1]):
                target.write_bytes(data)
                output = out / ('NO_OUTPUT-' + str(corruptions))
                result = subprocess.run([sys.executable, str(generator), str(case if directory == root else root),
                                         str(case if directory == worker else worker), str(output), '--packet-identity-boundary'],
                                        capture_output=True)
                assert result.returncode == 2 and name.encode() in result.stderr and not output.exists(), result.stderr
                corruptions += 1
    assert corruptions == 30
    role = here.parent / 'PioneerQ38TokenPacketIdentityBoundaryProgramsRole.h'
    base = dict.fromkeys(selectors, '1')
    positive = negative = 0

    def include(path):
        return '#include "' + str(path) + '"\n'

    def syntax(values, okay, body=None):
        global positive, negative
        source = ''.join('#define ' + key + ' ' + value + '\n' for key, value in sorted(values.items()))
        source += body if body is not None else include(role) + include(role)
        result = subprocess.run([*common, '-fsyntax-only', '-x', 'c++', '-'],
                                input=source, text=True, capture_output=True)
        assert (result.returncode == 0) == okay, (values, body, result.stderr)
        if okay:
            positive += 1
        else:
            assert 'error:' in result.stderr
            negative += 1

    syntax(base, True)
    syntax(base, True, include(committed) + include(committed)
           + 'static_assert(PioneerQ38TokenPacketIdentityBoundaryPrograms::PacketIdentityBoundary==1);\n')
    for key in selectors:
        for value in (None, '0', '2'):
            bad = base.copy()
            if value is None:
                del bad[key]
            else:
                bad[key] = value
            syntax(bad, False, include(committed))
        syntax(base, False, include(committed) + '#undef ' + key + '\n#define ' + key + ' 0\n' + include(committed))
    foreign = set(re.findall(r'defined\(([A-Z_][A-Z0-9_]*)\)', role.read_text())) - set(selectors)
    for key in sorted(foreign):
        for value in ('0', '1'):
            syntax({**base, key: value}, False)
        syntax(base, False, include(role) + '#define ' + key + ' 0\n' + include(role))
    selected = here.parent / 'PioneerQ38TokenSelectedPrograms.h'
    syntax(base, False, include(selected))
    syntax({}, False, include(selected) + ''.join('#define ' + key + ' 1\n' for key in selectors) + include(role))
    syntax(base, False, include(here.parent / 'PioneerQ38TokenCohortLocalSourcesPrograms.h') + include(role))
    syntax(base, False, include(committed) + include(here.parent / 'PioneerQ38TokenCohortLocalSourcesPrograms.h') + include(role))
    print(f'PASS packet-identity-boundary DATA/15 authenticated artifacts/30 corrupt+truncated refusals/{positive} syntax positives/'
          f'{negative} missing+zero+two+late+native+USER+ROOT BUILD+CPU+Work refusals;wholeoldcohort+commit+retained+frontier unchanged;'
          'NO native lifecycle,original model,physical,performance or Work grant')
