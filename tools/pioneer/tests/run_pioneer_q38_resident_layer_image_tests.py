#!/usr/bin/env python3
"""ALL actual NEW complete-layer ROM+retained peer programs;NO hardware."""
import os
import pathlib
import re
import subprocess
import sys
import tempfile
from pioneer_q38_resident_layer_image_common import verify

if len(sys.argv) not in (3, 4) or len(sys.argv) == 4 and sys.argv[3] != '--preflight-faults':
    raise SystemExit(2)
here = pathlib.Path(__file__).resolve().parent
image, peer = map(pathlib.Path, sys.argv[1:3])
verify(image, peer)
flags = re.findall(r'-D[A-Z][A-Z0-9_]*(?:=[0-9]+)?', (here/'run_pioneer_q38_ffn_persistent_image_tests.sh').read_text())
with tempfile.TemporaryDirectory(prefix='pioneer-layer-full-test-', dir='/private/tmp') as temporary:
    binary = pathlib.Path(temporary)/'test'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-ffp-contract=off',
                    '-Wall', '-Wextra', '-Werror', '-fsanitize=address,undefined', *flags,
                    str(here/'pioneer_q38_resident_layer_image_test.cpp'), '-o', str(binary)], check=True)
    subprocess.run([str(binary), *sys.argv[1:]], check=True,
                   env={**os.environ, 'ASAN_OPTIONS':'quarantine_size_mb=32', 'UBSAN_OPTIONS':'halt_on_error=1'})
