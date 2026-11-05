#!/usr/bin/env python3
"""Closed linked coordinator/worker instruction tests; no native/device IO."""
import ast
import hashlib
import os
import pathlib
import re
import subprocess
import sys
import tempfile
from compare_pioneer_q38_observed_trace import require, stable_bytes

require(__debug__ and not os.environ.get('PYTHONOPTIMIZE'), 'no optimized Python')
require(len(sys.argv) == 3, 'ABS_PREPARED_ROOT ABS_PREPARED_WORKER')
require(not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ), 'no role override')
here = pathlib.Path(__file__).resolve().parent
root_image, worker = map(pathlib.Path, sys.argv[1:])
fixture = worker / 'unchanged-original/unchanged-vector/unchanged-wide/unchanged-twin/unchanged-batched/unchanged-prefetch/unchanged-beta/unchanged-pushed/unchanged-precise/unchanged-global/qualified-math'
tool = '/Volumes/HaikuBuildLocal/generated.riscv64/cross-tools-riscv64/bin/riscv64-unknown-haiku-'


def pins(script):
    return next(ast.literal_eval(n.value) for n in ast.parse(script.read_text()).body
                if isinstance(n, ast.Assign) and any(isinstance(t, ast.Name) and t.id == 'PINS' for t in n.targets))


def snapshot(directory, records):
    require(directory.is_absolute() and directory.is_dir() and not directory.is_symlink(), 'absolute real ROM directory')
    require(bool(records), 'whole immutable ROM pins')
    result = []
    for name, size, digest in records:
        data = stable_bytes(directory / name, size)
        require(hashlib.sha256(data).hexdigest() == digest, 'whole ROM ' + name)
        result.append((name, data))
    return result


root = snapshot(root_image, [('program.elf', 194980, '0306100598ee1aa4e20bf3399165f5fbc2714ab2a694021fa98d63f4f64bb8f1')])
snapshots = [snapshot(directory, pins(script)) for directory, script in (
    (fixture, here / 'run_pioneer_q38_mixed_worker_tests.py'),
    (worker, here.parent / 'build_qwen38_prepared_exact_q4_worker.py'))]
legacy_pins = [(name + '.bin', size, digest) for name, size, digest in pins(here / 'run_pioneer_q38_resident_gdn_output_operator_service_image_tests.py')]
legacy_pins.append(('program.elf', 96148, '44bac0c1061f298ed8ccd2a1eb4780371bc7c80031cb230ad2da27f476202d19'))
legacy = snapshot(fixture / 'unchanged-worker', legacy_pins)
flags = re.findall(r'-D[A-Z][A-Z0-9_]*(?:=[0-9]+)?', (here / 'run_pioneer_q38_ffn_persistent_image_tests.sh').read_text())
with tempfile.TemporaryDirectory(prefix='pioneer-prepared-coordinator-', dir='/private/tmp') as temporary:
    base = pathlib.Path(temporary)
    images = []
    for i, records in enumerate([root, *snapshots]):
        directory = base / ('AUTHENTICATED_ROM_' + str(i))
        directory.mkdir(mode=0o700)
        for name, data in records:
            target = directory / name
            with target.open('xb') as stream:
                stream.write(data)
            target.chmod(0o400)
        if i == 0:
            for section in ('boot', 'text', 'terminal', 'conv', 'prepared'):
                target = directory / (section + '.bin')
                subprocess.run([tool + 'objcopy', '-O', 'binary', '-j', '.' + section,
                                str(directory / 'program.elf'), str(target)], check=True)
                section_bytes = target.read_bytes()
                require(section_bytes == stable_bytes(root_image / target.name, len(section_bytes)), 'whole ELF section ' + section)
                target.chmod(0o400)
        (directory / 'program.symbols').write_bytes(subprocess.check_output([tool + 'nm', '-n', str(directory / 'program.elf')]))
        (directory / 'program.symbols').chmod(0o400)
        images.append(directory)
    old = images[1] / 'unchanged-worker'
    old.mkdir(mode=0o700)
    for name, data in legacy:
        target = old / name
        with target.open('xb') as stream:
            stream.write(data)
        target.chmod(0o400)
    binary = base / 'test'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-ffp-contract=off',
                    '-Wall', '-Wextra', '-Werror', '-fsanitize=address,undefined', *flags,
                    str(here / 'pioneer_q38_prepared_q4_coordinator_instruction_test.cpp'), '-o', str(binary)], check=True)
    subprocess.run([str(binary), *map(str, images)], check=True,
                   env={**os.environ, 'ASAN_OPTIONS': 'quarantine_size_mb=32', 'UBSAN_OPTIONS': 'halt_on_error=1'})
print('CLOSED_PREPARED_COORDINATOR_INSTRUCTIONS whole immutable ROMs;NO complete ROOT/native/card/Assistant/model/TPS grant', flush=True)
