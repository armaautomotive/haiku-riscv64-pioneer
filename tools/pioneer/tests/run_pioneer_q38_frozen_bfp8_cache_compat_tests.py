#!/usr/bin/env python3
"""Build distinct frozen-writer/current-reader processes on synthetic data."""
import os
from pathlib import Path
import subprocess
import tempfile

here = Path(__file__).resolve().parent
root = here.parent
cxx = os.environ.get('CXX', 'c++')
common = [cxx, '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
          '-ffp-contract=off', '-fsanitize=address,undefined', '-I' + str(root)]
subprocess.run([os.environ.get('PYTHON', 'python3'), str(here / 'pioneer_q38_frozen_bfp8_recipe_test.py')], check=True)
with tempfile.TemporaryDirectory(prefix='pioneer-frozen-bfp8-compat-') as directory:
    base = Path(directory)
    source = str(here / 'pioneer_q38_frozen_bfp8_cache_compat_test.cpp')
    writer, reader = base / 'writer', base / 'reader'
    subprocess.run(common + ['-DPIONEER_TEST_FROZEN_BFP8_WRITER=1', source,
                   str(root / 'frozen_bfp8' / 'PioneerModelWeightCache.cpp'), '-o', str(writer)], check=True)
    subprocess.run(common + [source, str(root / 'PioneerModelWeightCache.cpp'), '-o', str(reader)], check=True)
    cache = str(base / 'cache')
    subprocess.run([str(writer), cache], check=True)
    subprocess.run([str(reader), cache], check=True)
print('PASS frozen BFP8 writer -> current reader cache compatibility; synthetic only')
