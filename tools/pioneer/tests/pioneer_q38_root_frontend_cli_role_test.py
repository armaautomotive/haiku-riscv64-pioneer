#!/usr/bin/env python3
"""Explicit ROOT raw-snapshot fixture gates;NO model/device execution."""
import os
import pathlib
import re
import subprocess
import sys
import tempfile

assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
here = pathlib.Path(__file__).resolve().parent
runner = here/'run_pioneer_q38_token_native_initialization_tests.py'
header = here/'pioneer_q38_token_native_root_frontend_role.h'
source = header.read_text()
required = set(re.findall(r'!defined\(([A-Z_][A-Z0-9_]*)\)\s*\|\|\s*\1\s*!=\s*1\b', source))
forbidden = set(re.findall(r'(?<!!)defined\(([A-Z_][A-Z0-9_]*)\)', source))-required
assert len(required) == 8 and len(forbidden) == 5 and not required & forbidden
accepted = refused = 0
with tempfile.TemporaryDirectory(prefix='pioneer-root-frontend-gates-', dir='/private/tmp') as temporary:
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
            assert 'Explicit G3 original first-layer/cancel/STOP/HOLD/full-END ROOT frontend TEST required' in result.stderr
        accepted += okay
        refused += not okay
    prefix = tmp/'must-not-exist.bin'
    common = ['/unopened/zero', '--execution-end', '/unopened/model', '/unopened/old-worker', '/unopened/g3', str(prefix)]
    selector = '--root-frontend-snapshots'
    cases = [
        (common+[selector], 'ROOT frontend TEST requires'),
        (common+['--pushed-context', selector, selector], 'duplicate ROOT frontend TEST selector'),
        (common+['--precise-context', selector], 'ROOT frontend TEST requires'),
        (common+['--precise-context', '--pushed-context', selector], 'QTP3 cannot mix with QTP2'),
        (common+['--pushed-context', selector, '--control-adapter'], 'private control TEST requires'),
        (common+['--pushed-context', '--hold-recheck-pending'], 'historical pending-PC selector retired'),
    ]
    for mode in ('--execution-end-reload', '--execution-first-layer', '--execution-hold-first-layer',
                 '--execution-end-full-flow', '--execution-full-flow', '--execution-readout-empty'):
        args = list(common);args[1] = mode
        cases.append((args+['--pushed-context', selector], 'ROOT frontend TEST requires'))
    before = sorted(p.relative_to(tmp).as_posix() for p in tmp.rglob('*'))
    for args, expected in cases:
        result = subprocess.run([sys.executable, str(runner), *args], text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        assert result.returncode and expected in result.stderr and not result.stdout, result.stderr
        assert not prefix.exists() and sorted(p.relative_to(tmp).as_posix() for p in tmp.rglob('*')) == before
    print(f'PASS ROOT frontend TEST gates/{accepted} role positive/{refused} missing+zero+two+foreign role refusals/{len(cases)} early CLI refusals/no output/compiler/runtime creation;NO native/model/physical/TPS grant')
