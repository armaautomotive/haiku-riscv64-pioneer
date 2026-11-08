#!/usr/bin/env python3
"""Exercise the real tag129 handler with a synthetic lifecycle and user copy."""
import os
from pathlib import Path
import subprocess
import tempfile

assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
here = Path(__file__).resolve().parent
owner = here.parent.parent.parent / 'src/add-ons/kernel/drivers/misc/PioneerQ38TokenNativeOwner.h'
source = owner.read_text()
begin = source.index('struct PioneerQ38TokenHiddenReadResult {')
end = source.index('};', begin) + 2
body = source[begin:end]
assert 'position=0,completedLayers=0,terminalKind=0,committed=0' in body
with tempfile.TemporaryDirectory(prefix='pioneer-qgw8-hidden-control-', dir='/private/tmp') as name:
    root = Path(name)
    (root / 'actual_hidden_result.inc').write_text(body + '\n')
    binary = root / 'test'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=address,undefined', '-I' + str(root),
                    str(here / 'pioneer_q38_qgw8_hidden_control_test.cpp'), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True,
                   env={**os.environ, 'ASAN_OPTIONS': 'quarantine_size_mb=32', 'UBSAN_OPTIONS': 'halt_on_error=1'})
print('PASS actual tag129 hidden handler with mock endpoints; no hardware or numerical qualification')
