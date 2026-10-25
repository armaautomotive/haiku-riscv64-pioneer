#!/usr/bin/env python3
"""Exact complete code DATA/ALL120 mapping proof, no card or SDK writes."""
import hashlib
import os
import pathlib
import runpy
import subprocess
import sys
import tempfile

assert len(sys.argv) == 2
here = pathlib.Path(__file__).resolve().parent
generator = runpy.run_path(str(here.parent/'generate_qwen38_resident_output_images.py'))
image = pathlib.Path(sys.argv[1])
peer = image/'exact-companion'
for name, (section, size, digest, suffix) in generator['PINS'].items():
    data = ((image if name.startswith('root-') else peer)/(section+'.bin')).read_bytes()
    assert len(data) == size and hashlib.sha256(data).hexdigest() == digest
    patch = generator['generate']([str(image), str(peer), name])
    source = patch.split('\n', 2)[2].rsplit('*** End Patch\n', 1)[0]
    expected = ''.join(line[1:]+'\n' for line in source.splitlines())
    assert (here.parents[2]/'src/add-ons/kernel/drivers/misc'/('PioneerQ38ResidentOutput'+suffix+'.h')).read_text() == expected
with tempfile.TemporaryDirectory(prefix='pioneer-output-images-', dir='/private/tmp') as temporary:
    binary = pathlib.Path(temporary)/'test'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=address,undefined', str(here/'pioneer_q38_resident_output_images_test.cpp'), '-o', str(binary)], check=True)
    subprocess.run([str(binary), str(image), str(peer)], check=True,
                   env={**os.environ, 'ASAN_OPTIONS':'quarantine_size_mb=32', 'UBSAN_OPTIONS':'halt_on_error=1'})
