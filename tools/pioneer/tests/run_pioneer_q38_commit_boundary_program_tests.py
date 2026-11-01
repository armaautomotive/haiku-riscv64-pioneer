#!/usr/bin/env python3
"""Exact immutable program DATA and refusal gates; NEVER native/device access."""
import hashlib
import os
import pathlib
import re
import runpy
import subprocess
import sys
import tempfile

assert len(sys.argv) == 5 and not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
root, worker, old_root, frontier = map(pathlib.Path, sys.argv[1:])
assert all(p.is_absolute() for p in (root, worker, old_root, frontier))
here = pathlib.Path(__file__).resolve().parent
generator = here.parent / 'generate_qwen38_token_programs.py'
helpers = runpy.run_path(str(generator))
saved = sys.argv[:]
try:
    # The ENTIRE old retained+frontier generated headers and old DATA/role
    # regressions pass first. No old pin or acceptance gate is updated.
    sys.argv = [str(here / 'run_pioneer_q38_retained_cohort_program_tests.py'),
                str(old_root), str(worker), str(frontier)]
    runpy.run_path(sys.argv[0], run_name='__main__')
finally:
    sys.argv = saved
inputs = ((root, helpers['pins'](here.parent / 'build_qwen38_commit_boundary_root.py', 'PINS')),
          (worker, helpers['pins'](here.parent / 'build_qwen38_precise_beta_worker_image.py', 'PINS')))
for directory, pins in inputs:
    helpers['authenticated'](directory, pins)
committed = here.parent / 'PioneerQ38TokenCommitBoundaryPrograms.h'
assert hashlib.sha256(committed.read_bytes()).hexdigest() == 'f17464e86b23df5210830446fa894980deba0f1f9a469fd9676db788190df182'
selectors = ('PIONEER_Q38_TOKEN_COMMIT_BOUNDARY_PROGRAMS', 'PIONEER_Q38_TOKEN_RETAINED_COHORT_PROGRAMS',
             'PIONEER_Q38_TOKEN_FRONTIER_PROGRAMS', 'PIONEER_Q38_TOKEN_JOB_BOOT_FOLD_PROGRAMS',
             'PIONEER_Q38_TOKEN_PRECISE_BETA_PROGRAMS', 'PIONEER_Q38_TOKEN_FAULT_PROGRAMS',
             'PIONEER_Q38_TOKEN_PUSHED_PROGRAMS')
with tempfile.TemporaryDirectory(prefix='pioneer-q38-commit-program-data-', dir='/private/tmp') as temporary:
    out = pathlib.Path(temporary)
    header = out / 'commit-data.h'
    subprocess.run([sys.executable, str(generator), str(root), str(worker), str(header), '--commit-boundary-seals'], check=True)
    assert header.read_bytes() == committed.read_bytes()
    common = [os.environ.get('CXX', 'c++'), '-std=c++17', '-Wall', '-Wextra', '-Werror', '-I' + str(here.parent)]
    binary = out / 'test'
    subprocess.run([*common, '-O1', '-g', '-fsanitize=address,undefined', *['-D' + k + '=1' for k in selectors],
                    '-DPIONEER_Q38_PROGRAM_DATA_TEST_COMMIT_BOUNDARY=1',
                    '-DPIONEER_Q38_PUSHED_PROGRAMS_HEADER="' + str(header) + '"',
                    str(here / 'pioneer_q38_pushed_programs_test.cpp'), '-o', str(binary)], check=True)
    subprocess.run([str(binary), str(root), str(worker)], check=True,
                   env={**os.environ, 'ASAN_OPTIONS': 'quarantine_size_mb=32', 'UBSAN_OPTIONS': 'halt_on_error=1'})
    corruptions = 0
    for directory, pins in inputs:
        for artifact, _, _ in pins:
            case = out / ('bad-' + str(corruptions))
            case.mkdir()
            for path in directory.iterdir():
                if path.is_file():
                    (case / path.name).symlink_to(path)
            target = case / artifact
            target.unlink()  # Only this fixture's exact freshly created symlink.
            original = (directory / artifact).read_bytes()
            corrupt = bytearray(original)
            corrupt[len(corrupt) // 2] ^= 1
            for data in (corrupt, original[:-1]):
                target.write_bytes(data)
                output = out / ('NO_OUTPUT-' + str(corruptions))
                result = subprocess.run([sys.executable, str(generator), str(case if directory == root else root),
                                         str(case if directory == worker else worker), str(output), '--commit-boundary-seals'],
                                        capture_output=True)
                assert result.returncode == 2 and artifact.encode() in result.stderr and not output.exists(), result.stderr
                corruptions += 1
    assert corruptions == 30
    role = here.parent / 'PioneerQ38TokenCommitBoundaryProgramsRole.h'
    base = dict.fromkeys(selectors, '1')
    positives = denied = 0

    def include(path):
        return '#include "' + str(path) + '"\n'

    def syntax(values, okay, body=None):
        global positives, denied
        source = ''.join('#define ' + key + ' ' + value + '\n' for key, value in sorted(values.items()))
        source += body if body is not None else include(role) + include(role)
        result = subprocess.run([*common, '-fsyntax-only', '-x', 'c++', '-'], input=source, text=True, capture_output=True)
        assert (result.returncode == 0) == okay, (values, body, result.stderr)
        if okay:
            positives += 1
        else:
            assert 'error:' in result.stderr
            denied += 1

    syntax(base, True)
    syntax(base, True, include(committed) + include(committed)
           + 'static_assert(PioneerQ38TokenCommitBoundaryPrograms::CommitBoundarySeals==1);\n')
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
    syntax({}, False, include(selected) + ''.join('#define ' + k + ' 1\n' for k in selectors) + include(role))
    syntax(base, False, include(committed) + include(here.parent / 'PioneerQ38TokenRetainedCohortPrograms.h') + include(role))
    print('PASS commit-boundary DATA/15 authenticated artifacts/30 corrupt+truncated refusals/'
          + str(positives) + ' syntax positives/' + str(denied)
          + ' missing+zero+two+late+native+USER+CPU+Work refusals/whole old retained+frontier controls unchanged;'
            'NO native selector,lifecycle,original model,physical,timing or Work grant')
