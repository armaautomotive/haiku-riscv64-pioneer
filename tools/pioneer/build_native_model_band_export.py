#!/usr/bin/env python3
"""Build CPU-only private fixed-model-band exporter;NO conversion/run/install."""
import argparse
import hashlib
import os
import pathlib
import subprocess
if not __debug__ or os.environ.get('PYTHONOPTIMIZE'):
    raise SystemExit('Assertions required')
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('output', type=pathlib.Path)
args = parser.parse_args()
assert args.output.is_absolute() and not args.output.exists() and not args.output.is_symlink()
assert not any(key.startswith(('PIONEER_', 'TT_PIONEER_')) for key in os.environ)
here = pathlib.Path(__file__).resolve().parent
args.output.mkdir(mode=0o700)
command = [os.environ.get('CXX', 'c++'), '-std=c++17', '-O2', '-g', '-Wall', '-Wextra', '-Werror', '-ffp-contract=off',
           str(here / 'pioneer_native_model_band_export.cpp'), str(here / 'PioneerModelWeightCache.cpp'),
           '-o', str(args.output / 'pioneer-native-model-band-export')]
subprocess.run(command, check=True)
(args.output / 'build-argv.txt').write_text('\n'.join(command) + '\n')
print(hashlib.sha256((args.output / 'pioneer-native-model-band-export').read_bytes()).hexdigest(), 'pioneer-native-model-band-export')
print('BUILD_ONLY CPU exporter;NO source/cache read,model conversion,device or private payload generated')
