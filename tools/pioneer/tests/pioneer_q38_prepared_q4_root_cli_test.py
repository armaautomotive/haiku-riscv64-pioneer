#!/usr/bin/env python3
"""Compact prepared-ROOT selector/authentication refusals; no device use."""
import os
import pathlib
import subprocess
import sys
import tempfile

if not __debug__ or os.environ.get('PYTHONOPTIMIZE'):
    raise SystemExit('Refuse optimized Python; CLI checks are required')
assert len(sys.argv) == 2
script = pathlib.Path(sys.argv[1]).resolve()
assert script.is_file()
assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)

with tempfile.TemporaryDirectory(prefix='pioneer-q38-prepared-root-cli-') as temp:
    root = pathlib.Path(temp)
    sdk = root / 'sdk-does-not-need-to-exist'
    out = root / 'out'

    def refuses(name, args, diagnostic):
        result = subprocess.run([sys.executable, str(script), str(sdk), str(out),
                                 '--inspect-candidate', *args],
                                stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                                text=True)
        assert result.returncode != 0, name
        assert diagnostic in result.stderr, (name, result.stderr)
        print('PASS_PREPARED_ROOT_CLI_REFUSAL ' + name, flush=True)

    refuses('frontier-requires-control',
            ['--compact-frontier-control', '--counter-unroll', '--phase-profile'],
            'compact QGW3 requires an explicit authenticated control directory')
    refuses('q3-requires-control',
            ['--compact-q3-control', '--counter-unroll', '--phase-profile'],
            'compact QGW3 requires an explicit authenticated control directory')
    refuses('selectors-mutually-exclusive',
            ['--compact-q3-control', '--compact-frontier-control', '--control',
             str(root), '--counter-unroll', '--phase-profile'],
            'assert args.count')

    # A directory with the right filenames but wrong bytes must not authorize
    # the new frontier pairing. The check is content-hash based, not presence.
    control = root / 'forged-control'
    control.mkdir()
    (control / 'program.elf').write_bytes(b'forged')
    (control / 'text.bin').write_bytes(b'forged')
    refuses('frontier-content-hash',
            ['--compact-frontier-control', '--control', str(control),
             '--counter-unroll', '--phase-profile'],
            'AssertionError')

print('PASS_PREPARED_ROOT_CLI_GATES refusals4;NO HARDWARE OR PERFORMANCE CLAIM',
      flush=True)
