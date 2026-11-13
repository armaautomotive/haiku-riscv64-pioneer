#!/usr/bin/env python3
"""Test recurrent receipt against real span core; simulated hardware only."""
import hashlib
import os
from pathlib import Path
import subprocess
import sys
import tempfile

assert __debug__ and not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
assert len(sys.argv) == 2
image = Path(sys.argv[1])
assert image.is_absolute() and image.is_dir()
assert hashlib.sha256((image / 'text.bin').read_bytes()).hexdigest() == '9b4e950f3d016d229b7f9ed7c5dd9f2e0fc23a2865a5f1a36f6633216f397609'
assert hashlib.sha256((image / 'boot.bin').read_bytes()).hexdigest() == 'e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b'
here = Path(__file__).resolve().parent
driver = here.parents[2] / 'src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp'
source = driver.read_text()
test = str(here / 'pioneer_q38_recurrent_q8_span_test.cpp')
common = [os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
          '-Wno-unused-function', '-ffp-contract=off', '-DTT_PIONEER_EXPERIMENTAL_Q38_BANKS']
with tempfile.TemporaryDirectory(prefix='recurrent-span-', dir='/private/tmp') as directory:
    out = Path(directory)
    for marker, name in (('PROBE_LOCK', 'pioneer_lock'), ('Q38_RETAIN_MEMORY', 'pioneer_retain_memory'),
                         ('Q38_SPAN_CONTROL', 'pioneer_span_control')):
        begin, end = '// PIONEER_' + marker + '_BEGIN', '// PIONEER_' + marker + '_END'
        assert source.count(begin) == source.count(end) == 1
        (out / (name + '.inc')).write_text(source.split(begin, 1)[1].split(end, 1)[0])
    flags = ['-I' + str(out)]
    binary = str(out / 'span')
    subprocess.run(common + flags + ['-fsanitize=address,undefined', test, '-o', binary], check=True)
    subprocess.run([binary, str(image)], check=True,
                   env=dict(os.environ, ASAN_OPTIONS='quarantine_size_mb=32', UBSAN_OPTIONS='halt_on_error=1'))
    for role in ('_KERNEL_MODE', 'PIONEER_DRIVER_BUILD_TAG', 'PIONEER_Q38_TOKEN_NATIVE_CONTEXT',
                 'PIONEER_Q38_WORK_INFERENCE', 'TT_PIONEER_Q38_WORK_MODE', 'PIONEER_Q38_CPU_VALIDATION',
                 'PIONEER_Q38_FULL_MODEL_CPU_REFERENCE'):
        assert subprocess.run(common + flags + ['-D' + role + '=1', '-E', test], capture_output=True).returncode, role
print('PASS recurrent span core; simulated hardware only')
