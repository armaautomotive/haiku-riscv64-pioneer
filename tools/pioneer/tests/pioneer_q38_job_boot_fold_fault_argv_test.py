#!/usr/bin/env python3
"""Compare old/current fault-mode subprocess contracts without executing C++."""
import copy
import hashlib
import os
import pathlib
import re
import subprocess
import sys
from unittest import mock

assert len(sys.argv) in (5, 7)
inputs = sys.argv[1:5]
assert all(pathlib.Path(p).is_absolute() for p in inputs)
assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
here = pathlib.Path(__file__).resolve().parent
runner = here/'run_pioneer_q38_job_boot_fold_connected_tests.py'
if len(sys.argv) == 7:
    assert sys.argv[5] == '--frozen-runner' and pathlib.Path(sys.argv[6]).is_absolute()
    frozen = pathlib.Path(sys.argv[6]).read_text()
else:
    frozen = subprocess.check_output(['git', 'show',
        'a449cc4e337405b6d5b2829502bfe9b8c2d88ddb:tools/pioneer/tests/run_pioneer_q38_job_boot_fold_connected_tests.py'], text=True)
assert hashlib.sha256(frozen.encode()).hexdigest() == '5bb30a6391652f2ac390472538bcf63e3d8182949afcd89d04313f178bde3134'
current = runner.read_text()

def normalize(value):
    if isinstance(value, str):
        return re.sub(r'/private/tmp/pioneer-job-boot-fold-connected-[^/]+', 'OWNED_TEST_TEMP', value)
    if isinstance(value, list):
        return [normalize(x) for x in value]
    if isinstance(value, tuple):
        return tuple(normalize(x) for x in value)
    if isinstance(value, dict):
        return {k: normalize(v) for k, v in value.items()}
    return value

def calls(source, mode):
    captured = []
    real_output = subprocess.check_output
    real_run = subprocess.run
    def record_run(argv, **kwargs):
        captured.append(normalize((copy.deepcopy(argv), copy.deepcopy(kwargs))))
        return subprocess.CompletedProcess(argv, 0)
    def read_output(argv, **kwargs):
        assert argv[0].endswith(('riscv64-unknown-haiku-nm', 'riscv64-unknown-haiku-objdump'))
        with mock.patch.object(subprocess, 'run', real_run):
            return real_output(argv, **kwargs)
    # nm/objdump and input hashes are real read-only checks. Only compiler
    # and test execution are intercepted; no model instructions run here.
    with mock.patch.object(sys, 'argv', [str(runner), *inputs, mode]), \
         mock.patch.object(subprocess, 'check_output', side_effect=read_output), \
         mock.patch.object(subprocess, 'run', side_effect=record_run):
        exec(compile(source, str(runner), 'exec'), {'__file__': str(runner), '__name__': '__main__'})
    assert len(captured) == 2 and '-fsanitize=address,undefined' in captured[0][0]
    assert captured[1][0] == ['OWNED_TEST_TEMP/test', *inputs]
    return captured

for mode in ('--retained-admission', '--completion-faults'):
    assert calls(frozen, mode) == calls(current, mode), mode
print('PASS old/current boot-fold fault modes: both compiler argv, executable argv and complete execution environments EXACT after owned-temporary-path normalization;real input pins+ELF metadata verified;compiler/model execution intercepted,NOT runtime/numeric/native/physical/TPS proof')
