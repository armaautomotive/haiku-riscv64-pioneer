#!/usr/bin/env python3
"""Compile QGWD control, receipt and manifest with the Haiku RISC-V ABI."""
from pathlib import Path
import subprocess
import sys
import tempfile

if len(sys.argv) != 2 or not Path(sys.argv[1]).is_absolute() or not Path(sys.argv[1]).is_dir():
    raise SystemExit('usage: run_pioneer_q38_qgwd_kernel_compile_tests.py ABS_HAIKU_BUILD')
sdk = Path(sys.argv[1])
tool = sdk / 'cross-tools-riscv64/bin/riscv64-unknown-haiku'
headers = sdk / 'objects/haiku/riscv64/packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop/headers'
here = Path(__file__).resolve().parent
source = str(here / 'pioneer_q38_qgwd_upload_control_kernel_compile.cpp')
common = [str(tool) + '-g++', '-std=gnu++17', '-O2', '-Wall', '-Wextra', '-Werror',
          '-Wno-misleading-indentation', '-fno-exceptions', '-fno-rtti', '-ffreestanding', '-fstack-usage',
          '-mcmodel=medany', '-fpic', '-D_KERNEL_MODE=1', '-I' + str(headers), '-idirafter', str(headers / 'posix')]
with tempfile.TemporaryDirectory(prefix='qgwd-kernel-compile-', dir='/private/tmp') as directory:
    obj = Path(directory) / 'control.o'
    subprocess.run(common + ['-c', source, '-o', str(obj)], check=True)
    frames = [line.split('\t') for line in obj.with_suffix('.su').read_text().splitlines()]
    assert frames and all(len(row) == 3 and row[2] == 'static' and int(row[1]) <= 2048 for row in frames)
    names = subprocess.check_output([str(tool) + '-nm', '--defined-only', str(obj)], text=True)
    assert all(symbol in names for symbol in ('qgwd_upload_control_compile', 'qgwd_dense_manifest_compile',
                                               'qgwd_bound_canonical_compile', 'qgwd_span_receipt_kind_compile',
                                               'qgwd_receipt_identity_compile','qgwd_native_binding_compile'))
    forbidden = {'api_version', 'init_hardware', 'init_driver', 'uninit_driver', 'publish_devices', 'find_device'}
    assert not any(line.split()[-1] in forbidden for line in names.splitlines() if line.split())
    for role in ('PIONEER_DRIVER_BUILD_TAG', 'PIONEER_Q38_TOKEN_NATIVE_CONTEXT', 'PIONEER_Q38_FULL_OFFLOAD_NATIVE',
                 'PIONEER_Q38_WORK_INFERENCE', 'TT_PIONEER_Q38_WORK_MODE', 'PIONEER_Q38_CPU_VALIDATION'):
        result = subprocess.run(common + ['-D' + role + '=1', '-fsyntax-only', source], capture_output=True)
        assert result.returncode, role
    print('PASS QGWD RISC-V kernel-shaped receipt/control/canonical/manifest compile, max static frame',
          max(int(row[1]) for row in frames), '; NOT A LOADABLE DRIVER')
