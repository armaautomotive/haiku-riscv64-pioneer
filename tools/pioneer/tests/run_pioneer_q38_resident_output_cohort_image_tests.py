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
    ('text', 128, 'c0569650df36e13b51c78b3ce8183bd0aba8e992ad4b5fc0361cb55d0081dd50'),
    ('conv', 6472, 'f1816502e3095391503e335bad44b78cf92389cb1528fa8992a014a6b632288a'),
    ('matrix', 14136, '1d2e2e7e4c8f806ff2983e5d9a2ccbd1a4d177940c7e23d73b6de33bae7e7559'),
    ('cohort', 50692, '5c9111c6cbf897f6a713923bef8a35e74da37120b94b632c0fd29690333017ce'),
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
