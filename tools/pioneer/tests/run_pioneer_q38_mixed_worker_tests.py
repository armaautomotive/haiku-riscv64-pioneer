#!/usr/bin/env python3
"""Actual emitted mixed worker, ASan+UBSan; no hardware IO."""
import os
import hashlib
import pathlib
import re
import subprocess
import sys
import tempfile
if len(sys.argv) not in (2, 3) or len(sys.argv) == 3 and sys.argv[2] != '--full':
    raise SystemExit(2)
here = pathlib.Path(__file__).resolve().parent
image = pathlib.Path(sys.argv[1])
PINS = (
    ('boot.bin', 4, 'a12af9d63593a1f882750a147dd7a511ac476c5c7a55526bd4d51c685173991c'),
    ('mixed.bin', 4400, 'daae5eab80a4e423d9b33a1eff84b39de6d29b7b4d85fe52a7e00e24bc3178f2'),
    ('dense.bin', 10332, 'cef405cc7405eea6001e47e9a6c9db73ca790bcac985f09d2bb964de8c68045f'),
    ('text.bin', 5984, '3a8ffe4f7e885f992f210bd1cc0887a1309159972409403b3082535b1916dab1'),
    ('affine.bin', 5360, '47c52fee6ccfb0a8847fe17a9d43d4016e7cfc6c68cc1427673f6869cd287b78'),
    ('ffn.bin', 16456, 'f6ef5d614091b5ed12365e1945daa2dad04d37c432ac9dd5f859f857bba83e52'),
    ('service.bin', 9888, 'b9fb256a75546dfe9be4a7805ef170913b2b1eecab1fbf1afb3b6d7c098c9262'),
    ('gdn.bin', 35564, 'eeaa084e659efbc5f2b7c9922a9de862057b2ed624bf85b03b8ccde2c02c5c74'),
    ('program.elf', 121680, '6a74556d6f4095353b0bffd0f44fe3d35706c21ea304f5233d034e47702117ea'),
)
for name, size, digest in PINS:
    data = (image/name).read_bytes()
    assert len(data) == size and hashlib.sha256(data).hexdigest() == digest, name
flags = re.findall(r'-D[A-Z][A-Z0-9_]*(?:=[0-9]+)?', (here/'run_pioneer_q38_ffn_persistent_image_tests.sh').read_text())
with tempfile.TemporaryDirectory(prefix='pioneer-mixed-worker-test-', dir='/private/tmp') as tmp:
    binary = pathlib.Path(tmp)/'test'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-ffp-contract=off', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=address,undefined', *flags, str(here/'pioneer_q38_mixed_worker_test.cpp'), '-o', str(binary)], check=True)
    subprocess.run([str(binary), *sys.argv[1:]], check=True,
                   env={**os.environ, 'ASAN_OPTIONS':'quarantine_size_mb=32', 'UBSAN_OPTIONS':'halt_on_error=1'})
