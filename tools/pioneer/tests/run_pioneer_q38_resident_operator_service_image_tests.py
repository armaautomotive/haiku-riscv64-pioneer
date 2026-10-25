#!/usr/bin/env python3
"""Exact new service bytes then ASan/UBSan emitted-program execution;NO card."""
import hashlib
import os
import pathlib
import re
import subprocess
import sys
import tempfile

if len(sys.argv) not in (3, 4) or len(sys.argv) == 4 and sys.argv[3] != '--smoke':
    raise SystemExit(2)
here = pathlib.Path(__file__).resolve().parent
image, old = map(pathlib.Path, sys.argv[1:3])
for section, size, digest in (
    ('boot', 4, 'e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b'),
    ('text', 7868, 'ea6245c426a2707f76dc58cf329d838f40d4dd0b48ff4b4509fffc9b5122a1fb'),
    ('affine', 5276, '8c496193e749e8933e927f82c7785b097bc375f54f61971e4c7f454efc8bbaca'),
    ('ffn', 16352, 'f17def1072f3e9ceaff2a7f1f66596ca34efc4d7ed9e3ea0d52dbc4cf9a7acb9'),
    ('service', 10300, '1e015c2bea109f36a08da5a651e582d9d8aee81e5c4f29a7eda7eb614d702c92'),
):
    data = (image/(section+'.bin')).read_bytes()
    assert len(data) == size and hashlib.sha256(data).hexdigest() == digest
for section, digest in (
    ('text', '783bd17e5f1114365bac3e7fa0589f205d70a225a2223a4de7089503ef90a615'),
    ('affine', 'a55331eeab74b50a50012a62c0d476e9a247f653eefe2211a08d0a75b2da85ca'),
    ('ffn', 'f17def1072f3e9ceaff2a7f1f66596ca34efc4d7ed9e3ea0d52dbc4cf9a7acb9'),
):
    assert hashlib.sha256((old/(section+'.bin')).read_bytes()).hexdigest() == digest
# Use the identical independently qualified integer/Tensix emulator model,
# NOT new numerical shortcuts or products fed into the returning program.
flags = re.findall(r'-D[A-Z][A-Z0-9_]*(?:=[0-9]+)?', (here/'run_pioneer_q38_ffn_persistent_image_tests.sh').read_text())
with tempfile.TemporaryDirectory(prefix='pioneer-resident-service-test-', dir='/private/tmp') as temporary:
    binary = pathlib.Path(temporary)/'test'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-ffp-contract=off',
                    '-Wall', '-Wextra', '-Werror', '-fsanitize=address,undefined', *flags,
                    str(here/'pioneer_q38_resident_operator_service_image_test.cpp'), '-o', str(binary)], check=True)
    subprocess.run([str(binary), *sys.argv[1:]], check=True,
                   env={**os.environ, 'ASAN_OPTIONS':'quarantine_size_mb=32', 'UBSAN_OPTIONS':'halt_on_error=1'})
