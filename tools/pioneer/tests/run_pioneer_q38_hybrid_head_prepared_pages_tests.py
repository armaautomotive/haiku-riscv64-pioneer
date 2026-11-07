#!/usr/bin/env python3
"""Test QGW8 preparation and format routing without a model or device."""
import os
from pathlib import Path
import subprocess
import tempfile

here = Path(__file__).resolve().parent
tools = here.parent
cxx = os.environ.get('CXX', 'c++')
common = [cxx, '-std=c++17', '-Wall', '-Wextra', '-Werror']
actual = str(here / 'pioneer_q38_hybrid_head_prepared_actual_compile.cpp')

with tempfile.TemporaryDirectory(prefix='pioneer-qgw8-prepared-test-') as tmp:
    binary = str(Path(tmp) / 'prepared-pages-test')
    subprocess.run(common + ['-O1', '-g', '-Wno-unused-function', '-ffp-contract=off',
        '-fsanitize=address,undefined', str(here / 'pioneer_q38_hybrid_head_prepared_pages_test.cpp'),
        str(tools / 'PioneerModelWeightCache.cpp'), '-o', binary], check=True)
    subprocess.run([binary], check=True)
    subprocess.run(common + ['-fsyntax-only', actual], check=True)
    fingerprint = '0123456789abcdef' * 4
    for name, role, macro, modes in (
        ('pioneer_q38_hybrid_bfp4_prepare.cpp', 'PIONEER_Q38_BFP4_PREPARATION_DIAGNOSTIC',
         'PIONEER_BFP4_IMPLEMENTATION_SHA256', ('--prepare-bfp4', '--verify-bfp4',
          '--prepare-qgw8-bfp4', '--verify-qgw8-bfp4')),
        ('pioneer_q38_head_bfp8_prepare.cpp', 'PIONEER_Q38_HEAD_BLOCK_FLOAT_DIAGNOSTIC',
         'PIONEER_HEAD_BFP8_IMPLEMENTATION_SHA256', ('--prepare-head-bfp8', '--verify-head-bfp8'))):
        cli = str(Path(tmp) / name.removesuffix('.cpp'))
        subprocess.run(common + ['-D' + role + '=1', '-D' + macro + '="' + fingerprint + '"',
            str(tools / name), str(tools / 'PioneerModelWeightCache.cpp'), '-o', cli], check=True)
        # These must reject arguments before opening the model or a cache.
        for args in ([], ['--unknown', '/private/tmp/cache'], *([mode, 'relative'] for mode in modes)):
            result = subprocess.run([cli, *args], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            if result.returncode != 2:
                raise RuntimeError(name + ' accepted invalid arguments: ' + repr(args))
    for forbidden in ('_KERNEL_MODE', 'PIONEER_Q38_WORK_INFERENCE', 'TT_PIONEER_Q38_WORK_MODE',
                      'PIONEER_Q38_CPU_VALIDATION', 'PIONEER_Q38_FULL_MODEL_CPU_REFERENCE'):
        result = subprocess.run(common + ['-fsyntax-only', '-D' + forbidden, actual],
                                text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        if result.returncode == 0 or 'isolated CPU-only' not in result.stderr:
            raise RuntimeError('Role guard failed for ' + forbidden + ': ' + result.stderr)

print('PASS QGW8 prepared pages and role guards; synthetic only, no model/device/inference')
