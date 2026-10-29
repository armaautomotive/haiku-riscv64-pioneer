#!/usr/bin/env python3
"""Wrong immutable original ROOT variants must refuse BEFORE model construction."""
import os
import pathlib
import subprocess
import sys
import tempfile
assert len(sys.argv) == 6 and not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
plain, cancel, old, worker, model = map(pathlib.Path, sys.argv[1:])
assert all(p.is_absolute() for p in (plain, cancel, old, worker, model))
runner = pathlib.Path(__file__).resolve().parent/'run_pioneer_q38_token_original_flow_tests.py'
with tempfile.TemporaryDirectory(prefix='pioneer-original-variant-', dir='/private/tmp') as tmp:
    for index, (root, mode) in enumerate(((plain, ['--cancel-boundary']), (cancel, []))):
        output = pathlib.Path(tmp)/f'NO_OUTPUT-{index}'
        result = subprocess.run([sys.executable, str(runner), str(root), str(old), str(worker), str(model),
                                 '--bootstrap-only', '--observed-trace', str(output), *mode], capture_output=True)
        assert result.returncode and b'text.bin' in result.stderr and b'START authenticate' not in result.stdout and not output.exists(), result.stderr
print('PASS exact plain-vs-cancellable original runner/2 wrong-image refusals BEFORE output directory/model construction;NOT a positive runtime or cancellation qualification')
