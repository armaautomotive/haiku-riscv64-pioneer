#!/usr/bin/env python3
"""Reject control-adapter fixture aliases before any input/output access."""
import os
import pathlib
import subprocess
import sys

assert not any(key.startswith(('PIONEER_', 'TT_PIONEER_')) for key in os.environ)
here = pathlib.Path(__file__).resolve().parent
runner = here/'run_pioneer_q38_token_native_initialization_tests.py'
never = here/'CONTROL_TEST_MUST_NOT_CREATE_OR_OPEN'
assert not never.exists() and not never.is_symlink()
base = str(never)
cases = [[], ['--held'], ['--runtime'], ['--bootstrap'], ['--execution'],
         ['--clock-owner-contract'], ['--preparation-endpoint-contract'],
         ['--execution-first-matrix', base, base, base],
         ['--execution-first-layer', base, base, base, base],
         ['--execution-full-flow', base, base, base, base],
         ['--execution-end-full-flow', base, base, base, base],
         ['--execution-end-state-full-flow', base, base, base, base],
         ['--execution-end', base, base, base, base, '--backend-finish']]
refused = 0
for args in cases:
    result = subprocess.run([sys.executable, str(runner), base, *args,
                             '--pushed-context', '--control-adapter'],
                            text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    assert result.returncode != 0 and not result.stdout
    assert 'private control TEST requires isolated early-init faults' in result.stderr, (args, result.stderr)
    assert not never.exists() and not never.is_symlink()
    refused += 1
result = subprocess.run([sys.executable, str(runner), base, '--control-adapter', '--control-adapter'],
                        text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
assert result.returncode != 0 and not result.stdout
assert "assert sys.argv.count('--control-adapter') == 1" in result.stderr
assert not never.exists() and not never.is_symlink()
refused += 1
print(f'PASS private control CLI/{refused} wrong-fixture+duplicate refusals BEFORE input/output access;'
      'NO model,device,lifecycle,Work,physical or TPS qualification')
