#!/usr/bin/env python3
"""ACTUAL emitted ROOT+119 ACTUAL companion programs,NO device."""
import hashlib
import os
import pathlib
import re
import subprocess
import sys
import tempfile

assert len(sys.argv) == 3 or len(sys.argv) == 4 and sys.argv[3] in ('--smoke', '--positives')
here = pathlib.Path(__file__).resolve().parent
image, worker = map(pathlib.Path, sys.argv[1:3])
for section, size, digest in (
    ('boot', 4, 'e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b'),
    ('text', 92, 'ce66ec25896f1e06362bae978499b33222c7c51d603ce36e83bf0e077b804c22'),
    ('conv', 6472, 'feb352acfc6490b727e32ba236d8bfa2e9f7b8811b31ae4ee37f5e4d53624112'),
    ('cohort', 14412, 'dc7c83eafe4f64ad03137ce6889abc9a5d27e5d3f25865520d960098f3d06b07'),
):
    data = (image/(section+'.bin')).read_bytes()
    assert len(data) == size and hashlib.sha256(data).hexdigest() == digest
for section, digest in (
    ('boot', 'e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b'),
    ('text', '47ff39e8f9709f9e87469e44919b374ccfbea3edd3f1f1f1b3020ecfd545c30b'),
    ('affine', 'd8734c6a6e1cea65e8abac370292033f8be1406fa063c0f84501646aaaedca5c'),
    ('ffn', 'f17def1072f3e9ceaff2a7f1f66596ca34efc4d7ed9e3ea0d52dbc4cf9a7acb9'),
    ('service', '3498f0507019f44b4c76101ca8df2df84dc4d57e3a288703c437fb441fa0eb43'),
    ('gdn', '67f202ea9afbf902488af532418c876c8983e146b21d272b1cbf88108b412648'),
):
    assert hashlib.sha256((worker/(section+'.bin')).read_bytes()).hexdigest() == digest
flags = re.findall(r'-D[A-Z][A-Z0-9_]*(?:=[0-9]+)?', (here/'run_pioneer_q38_ffn_persistent_image_tests.sh').read_text())
with tempfile.TemporaryDirectory(prefix='pioneer-cohort-root-test-', dir='/private/tmp') as temporary:
    binary = pathlib.Path(temporary)/'test'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-ffp-contract=off',
                    '-Wall', '-Wextra', '-Werror', '-fsanitize=address,undefined', *flags,
                    str(here/'pioneer_q38_resident_cohort_image_test.cpp'), '-o', str(binary)], check=True)
    subprocess.run([str(binary), *sys.argv[1:]], check=True,
                   env={**os.environ, 'ASAN_OPTIONS':'quarantine_size_mb=32', 'UBSAN_OPTIONS':'halt_on_error=1'})
