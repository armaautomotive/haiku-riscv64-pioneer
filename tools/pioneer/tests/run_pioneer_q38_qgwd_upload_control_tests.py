#!/usr/bin/env python3
"""Exercise the distinct QGWD control with real span service and fake OS."""
import hashlib
import os
from pathlib import Path
import subprocess
import sys
import tempfile

assert __debug__ and not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
if len(sys.argv) != 2:
    raise SystemExit('usage: run_pioneer_q38_qgwd_upload_control_tests.py ABS_FAST_IMAGE')
image = Path(sys.argv[1])
assert image.is_absolute() and image.is_dir()
assert hashlib.sha256((image / 'text.bin').read_bytes()).hexdigest() == '9b4e950f3d016d229b7f9ed7c5dd9f2e0fc23a2865a5f1a36f6633216f397609'
assert hashlib.sha256((image / 'boot.bin').read_bytes()).hexdigest() == 'e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b'
here = Path(__file__).resolve().parent
driver = here.parents[2] / 'src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp'
source = driver.read_text()
compiler = os.environ.get('CXX', 'c++')
common = [compiler, '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror', '-Wno-unused-function',
          '-ffp-contract=off', '-DTT_PIONEER_EXPERIMENTAL_Q38_BANKS']
test = str(here / 'pioneer_q38_qgwd_upload_control_test.cpp')
with tempfile.TemporaryDirectory(prefix='qgwd-control-', dir='/private/tmp') as directory:
    out = Path(directory)
    for marker, filename in (('PROBE_LOCK', 'pioneer_lock'), ('Q38_RETAIN_MEMORY', 'pioneer_retain_memory'),
                             ('Q38_SPAN_CONTROL', 'pioneer_span_control'), ('QGW8_CONTROL_DISPATCH', 'pioneer_qgw8_dispatch')):
        begin, end = '// PIONEER_' + marker + '_BEGIN', '// PIONEER_' + marker + '_END'
        assert source.count(begin) == 1 and source.count(end) == 1
        (out / (filename + '.inc')).write_text(source.split(begin, 1)[1].split(end, 1)[0])
    binary = str(out / 'control')
    flags = ['-I' + str(out)]
    subprocess.run(common + flags + ['-fsanitize=address,undefined', test, '-o', binary], check=True)
    subprocess.run([binary, str(image)], check=True,
                   env=dict(os.environ, ASAN_OPTIONS='quarantine_size_mb=32', UBSAN_OPTIONS='halt_on_error=1'))
    old = str(out / 'qgw8-regression')
    subprocess.run(common + flags + ['-fsanitize=address,undefined',
                   str(here / 'pioneer_q38_qgw8_upload_control_test.cpp'), '-o', old], check=True)
    subprocess.run([old, str(image)], check=True,
                   env=dict(os.environ, ASAN_OPTIONS='quarantine_size_mb=32', UBSAN_OPTIONS='halt_on_error=1'))
    for role in ('_KERNEL_MODE', 'PIONEER_DRIVER_BUILD_TAG', 'PIONEER_Q38_TOKEN_NATIVE_CONTEXT',
                 'PIONEER_Q38_WORK_INFERENCE', 'TT_PIONEER_Q38_WORK_MODE', 'PIONEER_Q38_CPU_VALIDATION'):
        result = subprocess.run(common + flags + ['-D'+role+'=1', '-E', test], capture_output=True)
        assert result.returncode, role
print('PASS isolated QGWD control roles; NO DEVICE')
