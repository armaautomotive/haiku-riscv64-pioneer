#!/usr/bin/env python3
"""Bounded experimental runner rejects mode widening before any output."""
import os
import pathlib
import subprocess
import sys
import tempfile

here = pathlib.Path(__file__).resolve().parent
runner = here/'run_pioneer_q38_job_boot_fold_connected_tests.py'
assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
cases = 0
with tempfile.TemporaryDirectory(prefix='pioneer-job-boot-fold-cli-', dir='/private/tmp') as tmp:
    root = pathlib.Path(tmp)
    sentinel = root/'sentinel'
    sentinel.write_bytes(b'unchanged\n')
    paths = [str(root/name) for name in ('root', 'old', 'worker', 'model')]
    link = root/'dangling-output'
    link.symlink_to(root/'absent-target')
    invalid = [[], paths, paths+['--unknown'], paths+['--retained-admission', '--retained-admission'],
               paths+['--completion-faults', '--retained-admission']]
    for mode in ('--retained-admission', '--completion-faults'):
        for extra in ('--first-layer-only', '--snapshot-first-layer', '--prepare-only', '--bootstrap-only',
                      '--fast-root-memory', '--eos-after-prompt', '--observed-trace', '--native', '--work'):
            invalid.append(paths+[mode, extra])
        for index in range(4):
            args = paths.copy()
            args[index] = 'relative-input'
            invalid.append(args+[mode])
    first = paths+['--first-layer-only', '--observed-trace', str(root/'fresh-prefix')]
    invalid += [paths+['--first-layer-only'], paths+['--first-layer-only', '--observed-trace'],
                paths+['--first-layer-only', '--unknown-prefix', str(root/'fresh-prefix')],
                paths+['--first-layer-only', '--observed-trace', 'relative-output'],
                paths+['--first-layer-only', '--observed-trace', str(root)],
                paths+['--first-layer-only', '--observed-trace', str(sentinel)],
                paths+['--first-layer-only', '--observed-trace', str(link)]]
    for extra in ('--retained-admission', '--completion-faults', '--first-layer-only', '--snapshot-first-layer',
                  '--prepare-only', '--bootstrap-only', '--fast-root-memory', '--eos-after-prompt',
                  '--observed-trace', '--full-flow', '--native', '--work'):
        invalid.append(first+[extra])
    for index in range(4):
        args = first.copy()
        args[index] = 'relative-input'
        invalid.append(args)
    for args in invalid:
        before = sorted(p.relative_to(root) for p in root.rglob('*'))
        result = subprocess.run([sys.executable, str(runner), *args], text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        assert result.returncode and 'AssertionError' in result.stderr, (args, result.stdout, result.stderr)
        assert before == sorted(p.relative_to(root) for p in root.rglob('*')) and sentinel.read_bytes() == b'unchanged\n'
        cases += 1
    for key in ('PIONEER_Q38_GLOBAL_CHAIN_PUSHED_COMPLETION', 'TT_PIONEER_Q38_WORK_MODE', 'PIONEER_TEST_ROOT_PROFILE_MAP'):
        before = sorted(p.relative_to(root) for p in root.rglob('*'))
        for args in (paths+['--retained-admission'], first):
            result = subprocess.run([sys.executable, str(runner), *args],
                                    env={**os.environ, key: '1'}, text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            assert result.returncode and 'AssertionError' in result.stderr
            assert before == sorted(p.relative_to(root) for p in root.rglob('*')) and sentinel.read_bytes() == b'unchanged\n'
            cases += 1
print(f'PASS job-boot-fold connected CLI/{cases} missing+unknown+duplicate+shortcut+relative+existing-or-symlink-output+role-override refusals/unchanged output inventory;HOST faults or incomplete first layer ONLY,not full model/native/Work')
