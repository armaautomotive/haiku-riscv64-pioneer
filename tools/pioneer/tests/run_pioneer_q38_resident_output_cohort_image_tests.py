#!/usr/bin/env python3
"""Pinned actual full Conv/GDN/QOL1/output matrix programs; NO card."""
import hashlib
import os
import pathlib
import re
import subprocess
import sys
import tempfile

if len(sys.argv) not in (3, 4) or len(sys.argv) == 4 and sys.argv[3] not in ('--smoke', '--positives', '--faults', '--terminal-faults'):
    raise SystemExit(2)
here = pathlib.Path(__file__).resolve().parent
image, peer = map(pathlib.Path, sys.argv[1:3])
PINS = (
    ('boot', 4, 'e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b'),
    ('text', 92, 'a5c7dc3a5349fddf0f27570c6a4ae1b22ad7b60ee418931235404eaf7a141aae'),
    ('conv', 6472, '6ed515323e4b3376f6e3f4bc8d78348643a6c758d822170fee799cec404a4694'),
    ('matrix', 14136, 'e86c2813892c807d6fe51c5f65fe58199169ff189d4abea15e2c96b89f4d9f6a'),
    ('cohort', 50692, '76958ca27a42af608d44fc6d3e9f4561853a61057b4d1b9889dc1acd0198bc7d'),
)
for section, size, digest in PINS:
    data = (image/(section+'.bin')).read_bytes()
    assert len(data) == size and hashlib.sha256(data).hexdigest() == digest
for section, digest in (
    ('boot', 'e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b'),
    ('text', 'd726337fe94d7d94812aaf5e67f3d4e328a21873cb27aa982dc5315bb6294b07'),
    ('affine', 'fcdf8ec5b607550db93fda5a86830951e2518d5580b11f4538fb6eead0eb3b7b'),
    ('ffn', 'f17def1072f3e9ceaff2a7f1f66596ca34efc4d7ed9e3ea0d52dbc4cf9a7acb9'),
    ('service', 'd00147de57e51e096760da0e6d25476692646807bcb0dc52d5a02b1263657254'),
    ('gdn', '36458af185c3500d1cd1b1170672c2138910aff836bd8a6a407b07c68a80b93d'),
):
    assert hashlib.sha256((peer/(section+'.bin')).read_bytes()).hexdigest() == digest
flags = re.findall(r'-D[A-Z][A-Z0-9_]*(?:=[0-9]+)?', (here/'run_pioneer_q38_ffn_persistent_image_tests.sh').read_text())
with tempfile.TemporaryDirectory(prefix='pioneer-output-cohort-test-', dir='/private/tmp') as temporary:
    binary = pathlib.Path(temporary)/'test'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-ffp-contract=off',
                    '-Wall', '-Wextra', '-Werror', '-fsanitize=address,undefined', *flags,
                    str(here/'pioneer_q38_resident_output_cohort_image_test.cpp'), '-o', str(binary)], check=True)
    subprocess.run([str(binary), *sys.argv[1:]], check=True,
                   env={**os.environ, 'ASAN_OPTIONS':'quarantine_size_mb=32', 'UBSAN_OPTIONS':'halt_on_error=1'})
