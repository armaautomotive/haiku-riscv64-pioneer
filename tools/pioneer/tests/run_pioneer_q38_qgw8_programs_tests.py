#!/usr/bin/env python3
"""Authenticate and pair QGW8 images off-device; never install a driver."""
import hashlib
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

if len(sys.argv) != 6 or any(not Path(p).is_absolute() or not Path(p).is_dir() for p in sys.argv[1:]):
    raise SystemExit('usage: script ABS_SDK ABS_ROOT_A ABS_ROOT_B ABS_WORKER_A ABS_WORKER_B')
if any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ):
    raise SystemExit('Pioneer role overrides refused')
sdk, root_a, root_b, worker_a, worker_b = map(Path, sys.argv[1:])
here = Path(__file__).resolve().parent
tools = here.parent
sys.path.insert(0, str(tools))
from PioneerQ38Qgw8ProgramPins import ROOT_PINS, WORKER_PINS

def run(*args, **kwargs):
    return subprocess.run([str(v) for v in args], check=True, **kwargs)

cxx = os.environ.get('CXX', 'c++')
generator = tools / 'generate_qwen38_hybrid_programs.py'
objcopy = sdk / 'cross-tools-riscv64/bin/riscv64-unknown-haiku-objcopy'
with tempfile.TemporaryDirectory(prefix='pioneer-qgw8-programs-', dir='/private/tmp') as temporary:
    tmp = Path(temporary)
    first, second = tmp / 'qgw8.h', tmp / 'qgw8-second.h'
    run(sys.executable, generator, '--qgw8-head-inspection', root_a, worker_a, first)
    run(sys.executable, generator, '--qgw8-head-inspection', root_b, worker_b, second)
    header = first.read_bytes()
    if second.read_bytes() != header:
        raise RuntimeError('independent QGW8 program packages differ')
    refused = subprocess.run([sys.executable, str(generator), '--qgw8-head-inspection',
                              str(root_a), str(worker_a), str(first)], capture_output=True)
    if refused.returncode == 0 or first.read_bytes() != header:
        raise RuntimeError('existing QGW8 output was accepted or changed')
    for directory, pins in ((root_a, ROOT_PINS), (worker_a, WORKER_PINS)):
        for name, size, digest in pins:
            data = (directory / name).read_bytes()
            if len(data) != size or hashlib.sha256(data).hexdigest() != digest:
                raise RuntimeError('artifact pin mismatch: ' + name)
            if name.endswith('.bin'):
                target = tmp / (directory.name + '-' + name)
                run(objcopy, '-O', 'binary', '-j', '.' + name[:-4], directory / 'program.elf', target)
                if target.read_bytes() != data:
                    raise RuntimeError('ELF/section mismatch: ' + name)
    damaged = tmp / 'bad-worker'
    damaged.mkdir()
    for name, _, _ in WORKER_PINS:
        shutil.copyfile(worker_a / name, damaged / name)
    target = damaged / 'globalworker.bin'
    data = target.read_bytes()
    target.write_bytes(bytes([data[0] ^ 1]) + data[1:])
    denied = tmp / 'denied.h'
    refused = subprocess.run([sys.executable, str(generator), '--qgw8-head-inspection', str(root_a),
                              str(damaged), str(denied)], capture_output=True)
    if refused.returncode == 0 or denied.exists():
        raise RuntimeError('modified QGW8 worker was accepted')
    flags = ['-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror', '-fsanitize=address,undefined',
             '-I' + str(tools), '-DPIONEER_Q38_QGW8_PROGRAMS_INSPECTION=1',
             '-DPIONEER_Q38_QGW8_LAYOUT_INSPECTION=1', '-DPIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION=1',
             '-DPIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION=1', '-DPIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT=1',
             '-DPIONEER_Q38_HYBRID_PROGRAMS_HEADER="' + str(first) + '"']
    image_test = tmp / 'image-test'
    run(cxx, *flags, here / 'pioneer_q38_hybrid_programs_test.cpp', '-o', image_test)
    env = dict(os.environ, ASAN_OPTIONS='quarantine_size_mb=32', UBSAN_OPTIONS='halt_on_error=1')
    run(image_test, root_a, worker_a, env=env)
    source = here / 'pioneer_q38_hybrid_programs_test.cpp'
    for remove in range(8, 13):
        result = subprocess.run([cxx, *flags[:remove], *flags[remove + 1:], '-fsyntax-only', str(source)], capture_output=True)
        if result.returncode == 0:
            raise RuntimeError('missing QGW8 program role accepted')
    for forbidden in ('_KERNEL_MODE', 'PIONEER_DRIVER_BUILD_TAG', 'PIONEER_Q38_TOKEN_NATIVE_CONTEXT',
                      'PIONEER_Q38_HYBRID_FULL_NATIVE', 'PIONEER_Q38_HYBRID_NATIVE_COMPILE_ONLY',
                      'PIONEER_Q38_FULL_OFFLOAD_NATIVE', 'PIONEER_Q38_FULL_OFFLOAD_CLIENT',
                      'PIONEER_Q38_BF16_FULL_NATIVE', 'PIONEER_Q38_BF16_FULL_CLIENT', 'PIONEER_Q38_WORK_INFERENCE',
                      'TT_PIONEER_Q38_WORK_MODE', 'PIONEER_Q38_CPU_VALIDATION', 'PIONEER_Q38_FULL_MODEL_CPU_REFERENCE'):
        result = subprocess.run([cxx, *flags, '-D' + forbidden, '-fsyntax-only', str(source)], capture_output=True)
        if result.returncode == 0:
            raise RuntimeError('forbidden QGW8 role accepted: ' + forbidden)
    selected = tmp / 'selected'
    selected_flags = ['-DPIONEER_Q38_QGW8_PROGRAMS_INSPECTION=1', '-DPIONEER_Q38_TOKEN_BF16_PROGRAMS=1',
                      '-DPIONEER_Q38_HYBRID_PROGRAMS_DIAGNOSTIC=1', '-DPIONEER_Q38_HYBRID_JOURNAL_DIAGNOSTIC=1',
                      '-DPIONEER_Q38_HYBRID_PROGRAMS_HEADER="' + str(first) + '"']
    # A deliberately wrong family must not be accepted as the matching
    # program even though its other inspection selectors are present.
    wrong = tmp / 'wrong-family.h'
    changed = header.replace(b'Family=0x38505451', b'Family=0x37505451').replace(
        b'Family==0x38505451', b'Family==0x37505451')
    if changed == header:
        raise RuntimeError('QGW8 family marker absent')
    wrong.write_bytes(changed)
    wrong_flags = selected_flags[:-1] + ['-DPIONEER_Q38_HYBRID_PROGRAMS_HEADER="' + str(wrong) + '"']
    rejected = subprocess.run([cxx, '-std=c++17', '-I' + str(tools), *wrong_flags, '-fsyntax-only',
                               str(here / 'pioneer_q38_qgw8_upload_receipt_test.cpp')], capture_output=True, text=True)
    if rejected.returncode == 0 or 'QGW8 inspection programs require their distinct identity' not in rejected.stderr:
        raise RuntimeError('wrong program family was not explicitly rejected')
    run(cxx, '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror', '-Wno-unused-function',
        '-Wno-unused-variable', '-fsanitize=address,undefined', '-I' + str(tools), *selected_flags,
        here / 'pioneer_q38_qgw8_upload_receipt_test.cpp', '-o', selected)
    run(selected, env=env)
print('PASS pinned QGW8 program pair, 120-worker code coverage and selected startup; synthetic only, no device/inference/TPS')
