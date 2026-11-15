#!/usr/bin/env python3
"""Ensure the three-product PV experiment cannot enter runtime roles."""
import os
import pathlib
import subprocess
import sys
import tempfile

if len(sys.argv) != 1:
    raise SystemExit(2)
here = pathlib.Path(__file__).resolve().parent
with tempfile.TemporaryDirectory(prefix='pioneer-qdm2-3pv-role-', dir='/private/tmp') as temporary:
    binary = pathlib.Path(temporary) / 'role-test'
    base = [os.environ.get('CXX', 'c++'), '-std=c++17', '-Wall', '-Wextra', '-Werror',
            '-I', str(here.parent), '-DPIONEER_Q38_DENSE_BF16_PV_3_PRODUCT_INSPECTION=1',
            '-o', str(binary), str(here / 'pioneer_q38_dense_matrix_three_product_role_test.cpp')]
    subprocess.run(base, check=True)
    subprocess.run([str(binary)], check=True)
    for role in ('PIONEER_Q38_WORK_INFERENCE=1', 'PIONEER_Q38_CPU_VALIDATION=1',
                 'PIONEER_DRIVER_BUILD_TAG=1', 'PIONEER_Q38_TOKEN_NATIVE_CONTEXT=1',
                 '_KERNEL_MODE=1'):
        failed = subprocess.run(base[:1] + ['-D' + role] + base[1:],
                                stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        assert failed.returncode != 0, role
print('PASS three-product QDM2 allowed only in offline inspection role; Work/validation/native/kernel roles rejected')
