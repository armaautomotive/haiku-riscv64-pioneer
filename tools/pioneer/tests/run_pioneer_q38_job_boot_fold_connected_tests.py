#!/usr/bin/env python3
"""Exact experimental ROOT, original bytes, existing connected fault suites.

HOST TEST only. No driver/native/Work route or installed firmware change.
The later boot-fold image is distinct from the eight-way first-layer image.
"""
import ast
import hashlib
import os
import pathlib
import re
import subprocess
import sys
import tempfile

assert len(sys.argv) == 6, 'four absolute input paths and one explicit fault mode required'
assert sys.argv[5] in ('--retained-admission', '--completion-faults'), 'only unshortened connected fault TEST modes allowed'
assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ), 'no role/profile overrides'
image, old, worker, model = map(pathlib.Path, sys.argv[1:5])
assert all(p.is_absolute() and not p.is_symlink() for p in (image, old, worker, model)), 'absolute non-symlink inputs required'
here = pathlib.Path(__file__).resolve().parent
CANDIDATE_PINS = (
    ('boot.bin', 4, '32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca'),
    ('text.bin', 130960, '6cb8ae7d5b03525ec233aa22b2230c815cab311771c532e82ba7d7bd7ced9dff'),
    ('terminal.bin', 8, '6eb1d84addfac7202ad4f0d747c6e64d3b92c03025651c7dabe6c56c9cba95d5'),
    ('conv.bin', 32720, '72c325aaaf632e3fc9a5f7c1a0286387982021eec171705f9027c8b16868a09c'),
    ('program.elf', 190592, '430b89b2fb734bfb66bbe96cc1e8581fcd6476d263a2578b8e869936df4bc1c7'),
)
def pins(script):
    tree = ast.parse(script.read_text())
    return next(ast.literal_eval(n.value) for n in tree.body if isinstance(n, ast.Assign)
                and any(isinstance(t, ast.Name) and t.id == 'PINS' for t in n.targets))

for directory, expected in ((image, CANDIDATE_PINS),
                            (old, pins(here/'run_pioneer_q38_mixed_worker_tests.py')),
                            (worker, pins(here.parent/'build_qwen38_pushed_worker_image.py'))):
    assert expected
    for name, size, digest in expected:
        raw = (directory/name).read_bytes()
        assert len(raw) == size and hashlib.sha256(raw).hexdigest() == digest, ('exact boot-fold connected TEST inputs', directory, name)
# The original-model provider authenticates the SAME full opened GGUF before
# constructing ROOT, just as in the unchanged original connected tests.
assert model.is_file()
tool = '/Volumes/HaikuBuildLocal/generated.riscv64/cross-tools-riscv64/bin/riscv64-unknown-haiku-'
for directory in (image, worker):
    actual = subprocess.check_output([tool+'nm', '-n', str(directory/'program.elf')], text=True)
    assert actual == (directory/'program.symbols').read_text()
flags = re.findall(r'-D[A-Z][A-Z0-9_]*(?:=[0-9]+)?', (here/'run_pioneer_q38_ffn_persistent_image_tests.sh').read_text())
assert flags
source = ('pioneer_q38_connected_pushed_admission_test.cpp' if sys.argv[5] == '--retained-admission'
          else 'pioneer_q38_connected_pushed_completion_test.cpp')
intervals = {}
for line in subprocess.check_output([tool+'objdump', '-tC', str(image/'program.elf')], text=True).splitlines():
    p = line.split(None, 5)
    if len(p) != 6 or p[2] != 'F':
        continue
    start, size = int(p[0], 16), int(p[4], 16)
    if not size:
        continue
    assert not (start | size) & 3
    assert (0x40000 <= start and start+size <= 0x60000) or (0x64000 <= start and start+size <= 0x6c000)
    intervals.setdefault((start, size), p[5])
end = 0
for (start, size), name in sorted(intervals.items()):
    assert start >= end
    end = start+size
with tempfile.TemporaryDirectory(prefix='pioneer-job-boot-fold-connected-', dir='/private/tmp') as tmp:
    out = pathlib.Path(tmp)
    profile = out/'profile.map'
    profile.write_text(''.join(f'{start:x} {size:x} {name}\n' for (start, size), name in sorted(intervals.items())))
    binary = out/'test'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                    '-ffp-contract=off', '-fsanitize=address,undefined', *flags,
                    '-DPIONEER_Q38_TEST_CANCELLABLE_ROOT=1',
                    '-DPIONEER_Q38_GLOBAL_CHAIN_PRECISE_ACTIVATION=1',
                    '-DPIONEER_Q38_GLOBAL_CHAIN_PUSHED_COMPLETION=1',
                    str(here/source), str(here/'pioneer_q38_whole_original_bytes.cpp'), '-o', str(binary)], check=True)
    subprocess.run([str(binary), *sys.argv[1:5]], check=True,
                   env={**os.environ, 'ASAN_OPTIONS': 'quarantine_size_mb=32', 'UBSAN_OPTIONS': 'halt_on_error=1',
                        'PIONEER_TEST_ROOT_PROFILE_MAP': str(profile)})
