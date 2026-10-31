#!/usr/bin/env python3
"""Early CLI and TEST-role refusals; no model/device/native execution."""
import os
import pathlib
import re
import subprocess
import sys
import tempfile

assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
here = pathlib.Path(__file__).resolve().parent
runner = here/'run_pioneer_q38_token_native_initialization_tests.py'
header = here/'pioneer_q38_token_native_hold_recheck_role.h'
source = header.read_text()
required = set(re.findall(r'!defined\(([A-Z_][A-Z0-9_]*)\)\s*\|\|\s*\1\s*!=\s*1\b', source))
assert len(required) == 8
forbidden = set(re.findall(r'(?<!!)defined\(([A-Z_][A-Z0-9_]*)\)', source))-required
assert len(forbidden) == 4
accepted = refused = 0
with tempfile.TemporaryDirectory(prefix='pioneer-hold-recheck-gates-', dir='/private/tmp') as temporary:
    tmp = pathlib.Path(temporary)
    role_source = '#include "'+str(header)+'"\nint main(){}\n'
    base = dict.fromkeys(required, 1)
    cases = [(base, True), ({}, False)]
    for name in sorted(required):
        cases += [({k:v for k,v in base.items() if k != name}, False)]
        cases += [({**base, name:value}, False) for value in (0, 2)]
    for name in sorted(forbidden):
        cases += [({**base, name:value}, False) for value in (0, 1, 2)]
    for flags, okay in cases:
        result = subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-x', 'c++', '-fsyntax-only',
                                 *['-D'+k+'='+str(v) for k,v in sorted(flags.items())], '-'], input=role_source,
                                text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        assert (result.returncode == 0) == okay
        if not okay:
            assert 'Explicit G3 original first-layer/cancel/STOP/HOLD/full-END pending-PC TEST required' in result.stderr
        accepted += okay
        refused += not okay
    # Every refusal is before compiler/runtime/output creation. No bogus
    # model/zero/worker path is granted authority by the new option.
    prefix = tmp/'must-not-exist.bin'
    common = ['/unopened/zero', '--execution-end', '/unopened/model', '/unopened/old-worker', '/unopened/g3', str(prefix)]
    cases = [
        (common+['--hold-recheck-pending'], 'pending-recheck TEST requires'),
        (common+['--pushed-context', '--hold-recheck-pending', '--hold-recheck-pending'], 'duplicate pending-recheck TEST selector'),
        (common+['--precise-context', '--hold-recheck-pending'], 'pending-recheck TEST requires'),
        (common+['--precise-context', '--pushed-context', '--hold-recheck-pending'], 'QTP3 cannot mix with QTP2'),
    ]
    for mode in ('--execution-end-reload', '--execution-first-layer', '--execution-hold-first-layer',
                 '--execution-end-full-flow', '--execution-full-flow', '--execution-readout-empty'):
        args = list(common);args[1] = mode
        cases.append((args+['--pushed-context', '--hold-recheck-pending'], 'pending-recheck TEST requires'))
    before = sorted(p.relative_to(tmp).as_posix() for p in tmp.rglob('*'))
    for args, expected in cases:
        result = subprocess.run([sys.executable, str(runner), *args], text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        assert result.returncode and expected in result.stderr and not result.stdout
        assert not prefix.exists() and sorted(p.relative_to(tmp).as_posix() for p in tmp.rglob('*')) == before
    print(f'PASS HOLD pending-recheck TEST gates/{accepted} role positive/{refused} missing+zero+two+foreign role refusals/{len(cases)} early CLI refusals/no output or compiler/runtime creation;NO native/model/physical/TPS grant')
