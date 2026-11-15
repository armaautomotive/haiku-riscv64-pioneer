#!/usr/bin/env python3
"""Pinned all-layer output peer: actual emitted-program VM, NO card."""
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
PINS = (
    ('boot', 4, 'e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b'),
    ('text', 7940, '4347da7540fab8c0f4ad06289b148e40cdecdf1b24ad9179cf6e3d4827c134cd'),
    ('affine', 5360, 'fcdf8ec5b607550db93fda5a86830951e2518d5580b11f4538fb6eead0eb3b7b'),
    ('ffn', 16352, 'f17def1072f3e9ceaff2a7f1f66596ca34efc4d7ed9e3ea0d52dbc4cf9a7acb9'),
    ('service', 10300, 'd00147de57e51e096760da0e6d25476692646807bcb0dc52d5a02b1263657254'),
    ('gdn', 35292, '11b66775097b1c41deaa65717fa6fe674e5f5cbbd529ac38956ee1a8a6507dca'),
)
for section, size, digest in PINS:
    data = (image/(section+'.bin')).read_bytes()
    assert len(data) == size and hashlib.sha256(data).hexdigest() == digest
assert hashlib.sha256((old/'gdn.bin').read_bytes()).hexdigest() == '67f202ea9afbf902488af532418c876c8983e146b21d272b1cbf88108b412648'
flags = re.findall(r'-D[A-Z][A-Z0-9_]*(?:=[0-9]+)?', (here/'run_pioneer_q38_ffn_persistent_image_tests.sh').read_text())
with tempfile.TemporaryDirectory(prefix='pioneer-resident-gdn-output-test-', dir='/private/tmp') as temporary:
    binary = pathlib.Path(temporary)/'test'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-ffp-contract=off',
                    '-Wall', '-Wextra', '-Werror', '-fsanitize=address,undefined', *flags,
                    str(here/'pioneer_q38_resident_gdn_output_operator_service_image_test.cpp'), '-o', str(binary)], check=True)
    subprocess.run([str(binary), *sys.argv[1:]], check=True,
                   env={**os.environ, 'ASAN_OPTIONS':'quarantine_size_mb=32', 'UBSAN_OPTIONS':'halt_on_error=1'})
    # Qualify existing matrix/Conv/GDN operations on the NEW relocated image,
    # not just the unchanged control. This does not relax old pin gates.
    compatibility = pathlib.Path(temporary)/'compatibility'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-ffp-contract=off',
                    '-Wall', '-Wextra', '-Werror', '-fsanitize=address,undefined', *flags,
                    str(here/'pioneer_q38_resident_gdn_operator_service_image_test.cpp'), '-o', str(compatibility)], check=True)
    subprocess.run([str(compatibility), str(image), str(old/'unchanged-matrix-conv'), *sys.argv[3:]], check=True,
                   env={**os.environ, 'ASAN_OPTIONS':'quarantine_size_mb=32', 'UBSAN_OPTIONS':'halt_on_error=1'})
