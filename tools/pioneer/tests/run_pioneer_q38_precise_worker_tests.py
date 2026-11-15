#!/usr/bin/env python3
"""Pinned ACTUAL QGW2/QPA1 worker tests; no ROOT/native/device IO."""
import ast
import hashlib
import os
import pathlib
import re
import subprocess
import sys
import tempfile

assert len(sys.argv) == 3 and not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
old, image = map(pathlib.Path, sys.argv[1:])
here = pathlib.Path(__file__).resolve().parent
PINS = (
    ('boot.bin', 4, 'a12af9d63593a1f882750a147dd7a511ac476c5c7a55526bd4d51c685173991c'),
    # Rebuilt from the current L1-authoritative GDN shape sources; both
    # independent builds matched and this suite's ASan/UBSan semantic test
    # passed against the refreshed identity.
    ('globalworker.bin', 7124, 'ab8b9b27ff78bb9dbacbaaa15bab9677356f3bd8e9464d527ada46ea58976fa7'),
    ('precise.bin', 8392, '56c0c5296df33710019c5e6dde95ee26e95247e174a5a3b16a5ed8292bcf1d4e'),
    ('dense.bin', 10332, '2d137fa2c8fb2df3e13b348607f3216c5831744b369dcafcd9643cabe16dafe1'),
    ('text.bin', 5984, '2a5c8191751598c114215bd2d176eb444b65193f524bf4cfc5082e97fd8d188b'),
    ('affine.bin', 5360, '47c52fee6ccfb0a8847fe17a9d43d4016e7cfc6c68cc1427673f6869cd287b78'),
    ('ffn.bin', 16456, '0b48b4691f560d822a626c63a060473d28e9518be1ebdb13d6e304dadff8bfba'),
    ('service.bin', 9888, '3cf0bb2475eb1fce610202b75599a16b4b4697df52b95f2b64b952f0675ef982'),
    ('gdn.bin', 35468, '4f828d91a912c28331b27de21074aa014a2de36dc8e7ce058b94cfaaf24bdf24'),
    ('program.elf', 134956, '0cb406fbf85e36f7a4e952530f0e5ec097abc39dad438039e31b2d417c73e1f9'))
tree = ast.parse((here/'run_pioneer_q38_mixed_worker_tests.py').read_text())
old_pins = next(ast.literal_eval(n.value) for n in tree.body if isinstance(n, ast.Assign)
                and any(isinstance(t, ast.Name) and t.id == 'PINS' for t in n.targets))
for directory, pins in ((old, old_pins), (image, PINS)):
    assert directory.is_absolute() and directory.is_dir() and not directory.is_symlink()
    for name, size, digest in pins:
        path = directory/name
        assert path.is_file() and not path.is_symlink()
        data = path.read_bytes()
        assert len(data) == size and hashlib.sha256(data).hexdigest() == digest, (directory, name)
flags = re.findall(r'-D[A-Z][A-Z0-9_]*(?:=[0-9]+)?', (here/'run_pioneer_q38_ffn_persistent_image_tests.sh').read_text())
common = [os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-ffp-contract=off', '-Wall', '-Wextra', '-Werror',
          '-fsanitize=address,undefined']
with tempfile.TemporaryDirectory(prefix='pioneer-q38-precise-worker-', dir='/private/tmp') as temp:
    base = pathlib.Path(temp)
    # Independent posthoc sigmoid control is HOST-only, without old tensor
    # TEST flags (including TT_PIONEER_Q38_WORK_MODE). Never weaken its guard.
    bridge = base/'posthoc-sigmoid.o'
    subprocess.run([*common, '-c', str(here/'pioneer_q38_sigmoid_precision_host_bridge.cpp'), '-o', str(bridge)], check=True)
    binary = base/'test'
    subprocess.run([*common, *flags, str(here/'pioneer_q38_precise_worker_test.cpp'), str(bridge), '-o', str(binary)], check=True)
    subprocess.run([str(binary), str(old), str(image)], check=True,
                   env={**os.environ, 'ASAN_OPTIONS': 'quarantine_size_mb=32', 'UBSAN_OPTIONS': 'halt_on_error=1'})
