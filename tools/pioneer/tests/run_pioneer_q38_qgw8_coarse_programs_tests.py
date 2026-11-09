#!/usr/bin/env python3
"""Authenticate QGW8 coarse program DATA and reject native/Work aliases."""
import hashlib
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

assert len(sys.argv) == 5
assert not any(key.startswith(('PIONEER_', 'TT_PIONEER_')) for key in os.environ)
baseline, first_root, second_root, worker = map(Path, sys.argv[1:])
assert all(p.is_absolute() and p.is_dir() for p in (baseline, first_root, second_root, worker))
here = Path(__file__).resolve().parent
tools = here.parent
sys.path.insert(0, str(tools))
from PioneerQ38Qgw8CoarseProgramPins import ROOT_PINS, WORKER_PINS

generator = tools / 'generate_qwen38_hybrid_programs.py'
cxx = os.environ.get('CXX', 'c++')
def generate(*args):
    return subprocess.run([sys.executable, str(generator), *map(str, args)], capture_output=True, text=True)
with tempfile.TemporaryDirectory(prefix='qgw8-coarse-programs-', dir='/private/tmp') as name:
    tmp = Path(name)
    first, second, old = tmp / 'first.h', tmp / 'second.h', tmp / 'old.h'
    for root, output in ((first_root, first), (second_root, second)):
        result = generate('--qgw8-coarse-profile-inspection', root, worker, output)
        assert result.returncode == 0 and 'QGW8-COARSE' in result.stdout, result.stderr
    header = first.read_bytes()
    assert header == second.read_bytes()
    assert hashlib.sha256(header).hexdigest() == '1cb876f284e7850491f6c2569bd30fd58b6e9011ddd485e764a8b9b8025cd91f'
    assert b'Family=0x39505451' in header and b'Family==0x39505451' in header
    result = generate('--qgw8-native-candidate', baseline, worker, old)
    assert result.returncode == 0 and hashlib.sha256(old.read_bytes()).hexdigest() == '8ddeb0cfa01857557be9416fa61c8029040570cf67bd412e6dd1f59fdc32eecb'
    for directory, pins in ((first_root, ROOT_PINS), (worker, WORKER_PINS)):
        for artifact, size, digest in pins:
            data = (directory / artifact).read_bytes()
            assert len(data) == size and hashlib.sha256(data).hexdigest() == digest
    for selector, root in (('--qgw8-native-candidate', first_root), ('--qgw8-head-inspection', first_root),
                           ('--qgw8-coarse-profile-inspection', baseline)):
        denied = tmp / ('denied-' + str(len(list(tmp.iterdir()))) + '.h')
        result = generate(selector, root, tmp / 'MUST_NOT_OPEN_WORKER', denied)
        assert result.returncode and 'artifact text.bin' in result.stderr and 'FileNotFoundError' not in result.stderr and not denied.exists()
    denied = tmp / 'ambiguous.h'
    result = generate('--qgw8-coarse-profile-inspection', '--qgw8-native-candidate', first_root, worker, denied)
    assert result.returncode and not denied.exists()
    assert generate('--qgw8-coarse-profile-inspection', first_root, worker, first).returncode and first.read_bytes() == header
    damaged = tmp / 'damaged-worker'
    damaged.mkdir()
    for artifact, _, _ in WORKER_PINS:
        shutil.copyfile(worker / artifact, damaged / artifact)
    damaged_file = damaged / 'globalworker.bin'
    contents = damaged_file.read_bytes()
    damaged_file.write_bytes(bytes([contents[0] ^ 1]) + contents[1:])
    denied = tmp / 'bad-worker.h'
    assert generate('--qgw8-coarse-profile-inspection', first_root, damaged, denied).returncode and not denied.exists()
    roles = ('PIONEER_Q38_QGW8_COARSE_PROGRAMS_INSPECTION', 'PIONEER_Q38_QGW8_PROGRAMS_INSPECTION',
             'PIONEER_Q38_QGW8_LAYOUT_INSPECTION', 'PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION',
             'PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION', 'PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT')
    flags = ['-D' + role + '=1' for role in roles] + ['-DPIONEER_Q38_HYBRID_PROGRAMS_HEADER="' + str(first) + '"']
    common = [cxx, '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror', '-I' + str(tools)]
    source = str(here / 'pioneer_q38_hybrid_programs_test.cpp')
    subprocess.run(common + flags + ['-fsanitize=address,undefined', source, '-o', str(tmp / 'image-test')], check=True)
    subprocess.run([str(tmp / 'image-test'), str(first_root), str(worker)], check=True,
                   env={**os.environ, 'ASAN_OPTIONS': 'quarantine_size_mb=32', 'UBSAN_OPTIONS': 'halt_on_error=1'})
    for role in roles:
        flags_without = [flag for flag in flags if flag != '-D' + role + '=1']
        assert subprocess.run(common + flags_without + ['-fsyntax-only', source], capture_output=True).returncode, role
        assert subprocess.run(common + flags_without + ['-D' + role + '=0', '-fsyntax-only', source], capture_output=True).returncode, role
    for role in ('_KERNEL_MODE', 'PIONEER_DRIVER_BUILD_TAG', 'PIONEER_Q38_TOKEN_NATIVE_CONTEXT',
                 'PIONEER_Q38_HYBRID_FULL_NATIVE', 'PIONEER_Q38_QGW8_CANDIDATE_NATIVE_128',
                 'PIONEER_Q38_QGW8_CANDIDATE_CLIENT_128', 'PIONEER_Q38_QGW8_HIDDEN_NATIVE_129',
                 'PIONEER_Q38_QGW8_HIDDEN_CLIENT_129', 'PIONEER_Q38_HYBRID_NATIVE_COMPILE_ONLY',
                 'PIONEER_Q38_WORK_INFERENCE', 'TT_PIONEER_Q38_WORK_MODE',
                 'PIONEER_Q38_CPU_VALIDATION', 'PIONEER_Q38_FULL_MODEL_CPU_REFERENCE'):
        assert subprocess.run(common + flags + ['-D' + role + '=1', '-fsyntax-only', source], capture_output=True).returncode, role
print('PASS distinct QGW8 coarse program DATA, full 120-worker byte checks, identity/role/refusal tests; NO DEVICE/WORK/SPEED')
