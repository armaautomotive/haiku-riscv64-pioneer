#!/usr/bin/env python3
"""Actual combined pipeline plus independent completion/STOP proofs; no card."""
import hashlib
import os
import pathlib
import re
import runpy
import subprocess
import sys
import tempfile

assert len(sys.argv) == 3 and sys.argv[2] in ('0', '1', '2', '--all')
here = pathlib.Path(__file__).resolve().parent
image = pathlib.Path(sys.argv[1])
for name, (section, size, digest, _) in runpy.run_path(str(here.parent/'generate_qwen38_resident_output_images.py'))['PINS'].items():
    data = ((image if name.startswith('root-') else image/'exact-companion')/(section+'.bin')).read_bytes()
    assert len(data) == size and hashlib.sha256(data).hexdigest() == digest
flags = re.findall(r'-D[A-Z][A-Z0-9_]*(?:=[0-9]+)?', (here/'run_pioneer_q38_ffn_persistent_image_tests.sh').read_text())
with tempfile.TemporaryDirectory(prefix='pioneer-output-completion-', dir='/private/tmp') as temporary:
    binary = pathlib.Path(temporary)/'test'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-ffp-contract=off',
                    '-Wall', '-Wextra', '-Werror', '-fsanitize=address,undefined', *flags,
                    str(here/'pioneer_q38_resident_output_completion_test.cpp'), '-o', str(binary)], check=True)
    for variant in range(3) if sys.argv[2] == '--all' else [int(sys.argv[2])]:
        subprocess.run([str(binary), str(image), str(image/'exact-companion'), str(variant)], check=True,
                       env={**os.environ, 'ASAN_OPTIONS':'quarantine_size_mb=32', 'UBSAN_OPTIONS':'halt_on_error=1'})
