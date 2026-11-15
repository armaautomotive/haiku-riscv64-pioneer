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
    ('mixed.bin', 4400, 'c4993bbbab0098f3c8b18668fe5267ea8d722d10113958a36fbd5ad43262a9ec'),
    ('dense.bin', 10332, 'cef405cc7405eea6001e47e9a6c9db73ca790bcac985f09d2bb964de8c68045f'),
    ('text.bin', 5984, '3a8ffe4f7e885f992f210bd1cc0887a1309159972409403b3082535b1916dab1'),
    ('affine.bin', 5360, '47c52fee6ccfb0a8847fe17a9d43d4016e7cfc6c68cc1427673f6869cd287b78'),
    ('ffn.bin', 16456, 'f6ef5d614091b5ed12365e1945daa2dad04d37c432ac9dd5f859f857bba83e52'),
    ('service.bin', 9888, 'b9fb256a75546dfe9be4a7805ef170913b2b1eecab1fbf1afb3b6d7c098c9262'),
    ('gdn.bin', 35468, 'e22670e57fec1d16751ed2bbfb77f08278c962b89138e22c89a6210d8f880ff8'),
    ('program.elf', 121584, '56d5af734ae2d11577239aac470c46fc0300bf5c835eb4a9417974307819ffe6'),
)
REDISTRIBUTED_PINS = (
    ('boot.bin', 4, 'a12af9d63593a1f882750a147dd7a511ac476c5c7a55526bd4d51c685173991c'),
    ('mixed.bin', 4400, '96b7cdfa3785710704fcc6aca909722786e9e96dedb9a16d6d6ee7c22cba6451'),
    ('dense.bin', 10332, 'cef405cc7405eea6001e47e9a6c9db73ca790bcac985f09d2bb964de8c68045f'),
    ('text.bin', 5984, '3a8ffe4f7e885f992f210bd1cc0887a1309159972409403b3082535b1916dab1'),
    ('affine.bin', 5360, '47c52fee6ccfb0a8847fe17a9d43d4016e7cfc6c68cc1427673f6869cd287b78'),
    ('ffn.bin', 16456, 'f6ef5d614091b5ed12365e1945daa2dad04d37c432ac9dd5f859f857bba83e52'),
    ('service.bin', 9888, 'b9fb256a75546dfe9be4a7805ef170913b2b1eecab1fbf1afb3b6d7c098c9262'),
    ('gdn.bin', 40256, 'd97b6ceb60d469002c5216c15ae57eb10b34f027de965342c9e545357ca51c6d'),
    ('program.elf', 127224, '07ba54107f7f81cc1236954d01541cccdbb585bd80981b7e6a0e8ec737710858'),
)
for name, size, digest in PINS:
    data = (image/name).read_bytes()
    assert len(data) == size and hashlib.sha256(data).hexdigest() == digest, name
flags = re.findall(r'-D[A-Z][A-Z0-9_]*(?:=[0-9]+)?', (here/'run_pioneer_q38_ffn_persistent_image_tests.sh').read_text())
with tempfile.TemporaryDirectory(prefix='pioneer-mixed-worker-test-', dir='/private/tmp') as tmp:
    binary = pathlib.Path(tmp)/'test'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-ffp-contract=off', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=address,undefined', *flags, str(here/'pioneer_q38_mixed_worker_test.cpp'), '-o', str(binary)], check=True)
    subprocess.run([str(binary), str(image), *(('--full',) if len(sys.argv) == 3 and sys.argv[2] == '--full' else ())], check=True,
                   env={**os.environ, 'ASAN_OPTIONS':'quarantine_size_mb=32', 'UBSAN_OPTIONS':'halt_on_error=1'})
