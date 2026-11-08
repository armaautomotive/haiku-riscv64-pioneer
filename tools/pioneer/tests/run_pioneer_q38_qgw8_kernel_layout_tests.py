#!/usr/bin/env python3
"""Test QGW8 kernel geometry without loading a driver or accessing hardware."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile

if len(sys.argv) != 2 or not Path(sys.argv[1]).is_absolute() or not Path(sys.argv[1]).is_dir():
    raise SystemExit('usage: run_pioneer_q38_qgw8_kernel_layout_tests.py ABS_HAIKU_BUILD')
sdk = Path(sys.argv[1])
here = Path(__file__).resolve().parent
cxx = os.environ.get('CXX', 'c++')
common = [cxx, '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror']
probes = [str(here / 'pioneer_q38_qgw8_kernel_layout_role_probe.cpp'),
          str(here / 'pioneer_q38_qgw8_upload_receipt_role_probe.cpp')]
role = ['-DPIONEER_Q38_QGW8_LAYOUT_INSPECTION=1']
late_probe = str(here / 'pioneer_q38_qgw8_late_role_probe.cpp')
late = subprocess.run(common + ['-fsyntax-only', late_probe], text=True,
                      stdout=subprocess.PIPE, stderr=subprocess.PIPE)
if late.returncode == 0 or 'QGW8 inspection role must be selected before the canonical model header' not in late.stderr:
    raise RuntimeError('QGW8 late role selection not explicitly refused')
canonical_probe = str(here / 'pioneer_q38_qgw8_canonical_role_probe.cpp')
for flags in ([], role):
    subprocess.run(common + flags + ['-fsyntax-only', canonical_probe], check=True)
for flags in (['-DPIONEER_Q38_QGW8_LAYOUT_INSPECTION=0'], role + ['-D_KERNEL_MODE=1'],
              *(role + ['-D' + name] for name in ('PIONEER_DRIVER_BUILD_TAG', 'PIONEER_Q38_TOKEN_NATIVE_CONTEXT',
                'PIONEER_Q38_FULL_OFFLOAD_NATIVE', 'PIONEER_Q38_FULL_OFFLOAD_CLIENT',
                'PIONEER_Q38_BF16_FULL_NATIVE', 'PIONEER_Q38_BF16_FULL_CLIENT',
                'PIONEER_Q38_WORK_INFERENCE', 'TT_PIONEER_Q38_WORK_MODE',
                'PIONEER_Q38_CPU_VALIDATION', 'PIONEER_Q38_FULL_MODEL_CPU_REFERENCE'))):
    result = subprocess.run(common + flags + ['-fsyntax-only', canonical_probe], text=True,
                            stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if result.returncode == 0 or 'QGW8 canonical layout is non-loadable inspection data only' not in result.stderr:
        raise RuntimeError('QGW8 canonical role not refused: ' + repr(flags))
for probe in probes:
    subprocess.run(common + role + ['-fsyntax-only', probe], check=True)
    for flags in ([], ['-DPIONEER_Q38_QGW8_LAYOUT_INSPECTION=0'], role + ['-D_KERNEL_MODE=1'],
                  *(role + ['-D' + name] for name in ('PIONEER_DRIVER_BUILD_TAG', 'PIONEER_Q38_TOKEN_NATIVE_CONTEXT',
                    'PIONEER_Q38_FULL_OFFLOAD_NATIVE', 'PIONEER_Q38_FULL_OFFLOAD_CLIENT',
                    'PIONEER_Q38_BF16_FULL_NATIVE', 'PIONEER_Q38_BF16_FULL_CLIENT',
                    'PIONEER_Q38_WORK_INFERENCE', 'TT_PIONEER_Q38_WORK_MODE',
                    'PIONEER_Q38_CPU_VALIDATION', 'PIONEER_Q38_FULL_MODEL_CPU_REFERENCE'))):
        result = subprocess.run(common + flags + ['-fsyntax-only', probe], text=True,
                                stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        if result.returncode == 0 or 'non-loadable inspection data only' not in result.stderr:
            raise RuntimeError('QGW8 layout/receipt role not refused: ' + repr(flags))
span_probe = str(here / 'pioneer_q38_qgw8_span_service_role_probe.cpp')
span_role = role + ['-DPIONEER_Q38_QGW8_SPAN_INSPECTION=1']
subprocess.run(common + ['-fsyntax-only', span_probe], check=True)
subprocess.run(common + span_role + ['-fsyntax-only', span_probe], check=True)
for flags, reason in (
        (['-DPIONEER_Q38_QGW8_SPAN_INSPECTION=1'], 'distinct non-loadable tracked receipt role'),
        (span_role + ['-DPIONEER_TEST_OMIT_TRACKED'], 'distinct non-loadable tracked receipt role'),
        (role + ['-DPIONEER_Q38_QGW8_SPAN_INSPECTION=0'], 'distinct non-loadable tracked receipt role'),
        (span_role + ['-DPIONEER_TEST_QGW8_WORK_PAIR'], 'Unsupported tracked span receipt/Work pairing'),
        (span_role + ['-DPIONEER_TEST_QGW8_FALSE_FAST'], 'Unsupported tracked span receipt/Work pairing'),
        (span_role + ['-DPIONEER_TEST_LEGACY_QGW8_FAST'], 'Unsupported tracked span receipt/Work pairing')):
    result = subprocess.run(common + flags + ['-fsyntax-only', span_probe], text=True,
                            stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if result.returncode == 0 or reason not in result.stderr:
        raise RuntimeError('QGW8 span-service role not refused: ' + repr(flags))
with tempfile.TemporaryDirectory(prefix='pioneer-qgw8-kernel-layout-') as directory:
    out = Path(directory)
    test = str(out / 'layout-test')
    subprocess.run(common + ['-Wno-unused-function', '-Wno-unused-variable', '-fsanitize=address,undefined',
        str(here / 'pioneer_q38_qgw8_kernel_layout_test.cpp'), '-o', test], check=True)
    subprocess.run([test], check=True)
    receipt = str(out / 'receipt-test')
    subprocess.run(common + ['-Wno-unused-function', '-Wno-unused-variable', '-fsanitize=address,undefined',
        str(here / 'pioneer_q38_qgw8_upload_receipt_test.cpp'), '-o', receipt], check=True)
    for args in ([], ['--tamper'], ['--extra'], ['--abort'], ['--late'], ['--late-canonical']):
        subprocess.run([receipt, *args], check=True)
    image = out / 'span-image'
    subprocess.run(['sh', str(here.parent / 'build_qwen38_span_image.sh'), str(sdk), str(image)], check=True)
    span = str(out / 'tracked-span-test')
    subprocess.run(common + ['-ffp-contract=off', '-DTT_PIONEER_EXPERIMENTAL_Q38_BANKS',
        '-fsanitize=address,undefined', str(here / 'pioneer_q38_qgw8_tracked_span_test.cpp'), '-o', span], check=True)
    subprocess.run([span, str(image)], check=True)
    fast_image = out / 'fast-span-image'
    subprocess.run(['sh', str(here.parent / 'build_qwen38_span_image.sh'), str(sdk), str(fast_image)],
                   check=True, env=dict(os.environ, PIONEER_Q38_SPAN_FAST='1'))
    fast_span = str(out / 'tracked-fast-span-test')
    subprocess.run(common + ['-ffp-contract=off', '-DTT_PIONEER_EXPERIMENTAL_Q38_BANKS',
        '-DPIONEER_Q38_SPAN_FAST=1', '-DPIONEER_TEST_QGW8_FAST=1', '-fsanitize=address,undefined',
        str(here / 'pioneer_q38_qgw8_tracked_span_test.cpp'), '-o', fast_span], check=True)
    subprocess.run([fast_span, str(fast_image)], check=True)
    tool = sdk / 'cross-tools-riscv64/bin/riscv64-unknown-haiku'
    headers = sdk / 'objects/haiku/riscv64/packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop/headers'
    for name, symbols in (('kernel_layout', ('qgw8_kernel_layout_build',)),
                          ('upload_receipt', ('qgw8_receipt_begin', 'qgw8_receipt_span', 'qgw8_receipt_record',
                                              'qgw8_receipt_after', 'qgw8_receipt_seal', 'qgw8_receipt_layout',
                                              'qgw8_receipt_canonical', 'qgw8_runtime_ranges', 'qgw8_manifest')),
                          ('span_service', ('qgw8_span_receipt_kind',)),
                          ('startup', ('qgw8_startup_build', 'qgw8_startup_head'))):
        obj = out / (name + '.o')
        source = ('pioneer_q38_qgw8_kernel_layout_compile.cpp' if name == 'kernel_layout' else
                  'pioneer_q38_qgw8_upload_receipt_kernel_compile.cpp' if name == 'upload_receipt' else
                  'pioneer_q38_qgw8_startup_kernel_compile.cpp' if name == 'startup' else
                  'pioneer_q38_qgw8_span_service_role_probe.cpp')
        selectors = (['-DPIONEER_Q38_QGW8_LAYOUT_INSPECTION=1', '-DPIONEER_Q38_QGW8_SPAN_INSPECTION=1',
                      '-DPIONEER_Q38_HYBRID_KERNEL_COMPILE_ONLY=1'] if name == 'span_service' else [])
        subprocess.run([str(tool) + '-g++', '-std=gnu++17', '-O2', '-Wall', '-Wextra', '-Werror',
            '-Wno-misleading-indentation', '-fno-exceptions', '-fno-rtti', '-ffreestanding', '-fstack-usage',
            '-mcmodel=medany', '-fpic', '-D_KERNEL_MODE=1', *selectors, '-I' + str(headers),
            '-idirafter', str(headers / 'posix'), '-c', str(here / source), '-o', str(obj)], check=True)
        frames = [line.split('\t') for line in obj.with_suffix('.su').read_text().splitlines()]
        if not frames or any(len(row) != 3 or row[2] != 'static' or int(row[1]) > 2048 for row in frames):
            raise RuntimeError('Kernel layout/receipt stack usage refused')
        names = subprocess.check_output([str(tool) + '-nm', '--defined-only', str(obj)], text=True)
        if not all(symbol in names for symbol in symbols):
            raise RuntimeError('Compile-only layout/receipt witness missing')
        forbidden = {'api_version', 'init_hardware', 'init_driver', 'uninit_driver', 'publish_devices', 'find_device'}
        if any(line.split()[-1] in forbidden for line in names.splitlines() if line.split()):
            raise RuntimeError('Compile-only object unexpectedly exports a driver entry point')
        print('PASS QGW8 RISC-V', name, 'compile, max static frame', max(int(row[1]) for row in frames),
              '; no driver/hardware')
