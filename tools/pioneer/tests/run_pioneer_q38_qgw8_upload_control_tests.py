#!/usr/bin/env python3
"""Test non-loadable QGW8 control against simulated OS and span hardware."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile

if len(sys.argv) != 2 or not Path(sys.argv[1]).is_absolute() or not Path(sys.argv[1]).is_dir():
    raise SystemExit('usage: run_pioneer_q38_qgw8_upload_control_tests.py ABS_HAIKU_BUILD')
sdk = Path(sys.argv[1])
here = Path(__file__).resolve().parent
root = here.parents[2]
cxx = os.environ.get('CXX', 'c++')
role = ['-DPIONEER_Q38_QGW8_CONTROL_INSPECTION=1', '-DPIONEER_Q38_QGW8_SPAN_INSPECTION=1',
        '-DPIONEER_Q38_QGW8_LAYOUT_INSPECTION=1', '-DPIONEER_Q38_HYBRID_TRACKED_UPLOAD=1']
probe = str(here / 'pioneer_q38_qgw8_upload_control_role_probe.cpp')
common = [cxx, '-std=c++17', '-Wall', '-Wextra', '-Werror']
subprocess.run(common + role + ['-fsyntax-only', probe], check=True)
for flags in ([], ['-DPIONEER_Q38_QGW8_CONTROL_INSPECTION=0'],
              ['-DPIONEER_Q38_QGW8_CONTROL_INSPECTION=1'], role[:-1],
              *(role + ['-D' + name] for name in ('_KERNEL_MODE', 'PIONEER_DRIVER_BUILD_TAG', 'PIONEER_Q38_TOKEN_NATIVE_CONTEXT',
                'PIONEER_Q38_FULL_OFFLOAD_NATIVE', 'PIONEER_Q38_FULL_OFFLOAD_CLIENT',
                'PIONEER_Q38_BF16_FULL_NATIVE', 'PIONEER_Q38_BF16_FULL_CLIENT',
                'PIONEER_Q38_WORK_INFERENCE', 'TT_PIONEER_Q38_WORK_MODE',
                'PIONEER_Q38_CPU_VALIDATION', 'PIONEER_Q38_FULL_MODEL_CPU_REFERENCE'))):
    result = subprocess.run(common + flags + ['-fsyntax-only', probe], text=True,
                            stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if result.returncode == 0 or 'non-loadable inspection only' not in result.stderr:
        raise RuntimeError('QGW8 control role not refused: ' + repr(flags))
source = (root / 'src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp').read_text()
if source.count('#include "PioneerQ38Qgw8UploadControlInspection.inc"') != 1:
    raise RuntimeError('QGW8 handler must be included exactly once by the driver source')
with tempfile.TemporaryDirectory(prefix='pioneer-qgw8-control-') as name:
    out = Path(name)
    for marker, filename in (('PROBE_LOCK', 'pioneer_lock'), ('Q38_RETAIN_MEMORY', 'pioneer_retain_memory'),
                             ('Q38_SPAN_CONTROL', 'pioneer_span_control'), ('QGW8_CONTROL_DISPATCH', 'pioneer_qgw8_dispatch'),
                             ('QGW8_FULL_CONTROL_DISPATCH', 'pioneer_qgw8_full_dispatch')):
        begin, end = '// PIONEER_' + marker + '_BEGIN', '// PIONEER_' + marker + '_END'
        if source.count(begin) != 1 or source.count(end) != 1:
            raise RuntimeError('Missing or ambiguous existing driver fixture: ' + marker)
        (out / (filename + '.inc')).write_text(source.split(begin, 1)[1].split(end, 1)[0])
    image = out / 'image'
    subprocess.run(['sh', str(here.parent / 'build_qwen38_span_image.sh'), str(sdk), str(image)],
                   check=True, env=dict(os.environ, PIONEER_Q38_SPAN_FAST='1'))
    binary = out / 'control'
    subprocess.run(common + ['-O1', '-g', '-ffp-contract=off', '-Wno-unused-function', '-fsanitize=address,undefined',
        '-DTT_PIONEER_EXPERIMENTAL_Q38_BANKS', '-I' + str(out),
        str(here / 'pioneer_q38_qgw8_upload_control_test.cpp'), '-o', str(binary)], check=True)
    subprocess.run([str(binary), str(image)], check=True,
                   env=dict(os.environ, ASAN_OPTIONS='quarantine_size_mb=32', UBSAN_OPTIONS='halt_on_error=1'))
    routing = out / 'full-routing'
    subprocess.run(common + ['-I' + str(out), str(here / 'pioneer_q38_qgw8_full_routing_test.cpp'), '-o', str(routing)], check=True)
    subprocess.run([str(routing)], check=True)
    tool = sdk / 'cross-tools-riscv64/bin/riscv64-unknown-haiku'
    headers = sdk / 'objects/haiku/riscv64/packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop/headers'
    obj = out / 'control-kernel.o'
    subprocess.run([str(tool) + '-g++', '-std=gnu++17', '-O2', '-Wall', '-Wextra', '-Werror',
        '-Wno-misleading-indentation', '-fno-exceptions', '-fno-rtti', '-ffreestanding', '-fstack-usage',
        '-mcmodel=medany', '-fpic', '-D_KERNEL_MODE=1', '-I' + str(out), '-I' + str(headers), '-idirafter', str(headers / 'posix'),
        '-c', str(here / 'pioneer_q38_qgw8_upload_control_kernel_compile.cpp'), '-o', str(obj)], check=True)
    frames = [line.split('\t') for line in obj.with_suffix('.su').read_text().splitlines()]
    if not frames or any(len(row) != 3 or row[2] != 'static' or int(row[1]) > 2048 for row in frames):
        raise RuntimeError('QGW8 control kernel compile exceeded stack bound')
    names = subprocess.check_output([str(tool) + '-nm', '--defined-only', str(obj)], text=True)
    if not all(name in names for name in ('qgw8_upload_control_compile', 'qgw8_upload_control_dispatch_compile')):
        raise RuntimeError('QGW8 control compile-only witness missing')
    forbidden = {'api_version', 'init_hardware', 'init_driver', 'uninit_driver', 'publish_devices', 'find_device'}
    if any(line.split()[-1] in forbidden for line in names.splitlines() if line.split()):
        raise RuntimeError('QGW8 control object exports a driver entry')
    print('PASS non-loadable QGW8 RISC-V handler compile with kernel-shaped stubs; max static frame',
          max(int(row[1]) for row in frames))
print('PASS QGW8 separate control roles and simulated owner/span transaction; no driver or card')
