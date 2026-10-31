#!/usr/bin/env python3
"""Pinned HOST-only NoC cancel-accounting regression; no device access."""
import ast
import hashlib
import os
import pathlib
import re
import subprocess
import sys
import tempfile

fault_marker = sys.argv[4:] == ['--fault-marker']
assert (len(sys.argv) == 4 or fault_marker) and not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
image, old, worker = map(pathlib.Path, sys.argv[1:4])
assert all(p.is_absolute() for p in (image, old, worker))
here = pathlib.Path(__file__).resolve().parent
for directory, script, key in (
    (image, here.parent/('build_qwen38_token_fault_marker_image.py' if fault_marker else 'build_qwen38_token_root.py'),
     'PINS' if fault_marker else 'PINS_CANCEL_F32_PUSHED'),
    (old, here/'run_pioneer_q38_mixed_worker_tests.py', 'PINS'),
    (worker, here.parent/'build_qwen38_pushed_worker_image.py', 'PINS'),
):
    tree = ast.parse(script.read_text())
    pins = next(ast.literal_eval(n.value) for n in tree.body if isinstance(n, ast.Assign)
                and any(isinstance(t, ast.Name) and t.id == key for t in n.targets))
    assert pins
    for name, size, digest in pins:
        data = (directory/name).read_bytes()
        assert len(data) == size and hashlib.sha256(data).hexdigest() == digest, (directory, name)
tool = '/Volumes/HaikuBuildLocal/generated.riscv64/cross-tools-riscv64/bin/riscv64-unknown-haiku-'
for directory in (image, worker):
    assert subprocess.check_output([tool+'nm', '-n', str(directory/'program.elf')], text=True) == (directory/'program.symbols').read_text()
functions = []
for line in subprocess.check_output([tool+'objdump', '-tC', str(image/'program.elf')], text=True).splitlines():
    p = line.split(None, 5)
    if len(p) == 6 and p[2] == 'F' and p[5] == 'q38_pushed_end_shared':
        functions.append((int(p[0], 16), int(p[4], 16)))
assert len(functions) == 1 and functions[0][1]
start, size = functions[0]
flags = re.findall(r'-D[A-Z][A-Z0-9_]*(?:=[0-9]+)?', (here/'run_pioneer_q38_ffn_persistent_image_tests.sh').read_text())
with tempfile.TemporaryDirectory(prefix='pioneer-q38-host-cancel-counter-', dir='/private/tmp') as temporary:
    binary = pathlib.Path(temporary)/'test'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                    '-ffp-contract=off', '-fsanitize=address,undefined', *flags,
                    '-DPIONEER_Q38_TEST_CANCELLABLE_ROOT=1',
                    '-DPIONEER_Q38_GLOBAL_CHAIN_PRECISE_ACTIVATION=1',
                    '-DPIONEER_Q38_GLOBAL_CHAIN_PUSHED_COMPLETION=1',
                    str(here/'pioneer_q38_host_cancel_counter_test.cpp'), '-o', str(binary)], check=True)
    subprocess.run([str(binary), *sys.argv[1:4], f'{start:x}', f'{start+size:x}',
                    *(['--fault-marker'] if fault_marker else [])], check=True,
                   env={**os.environ, 'ASAN_OPTIONS':'quarantine_size_mb=32', 'UBSAN_OPTIONS':'halt_on_error=1'})
