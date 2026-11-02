#!/usr/bin/env python3
"""Execute whole frozen old/manual-wait workers against independent endpoints.

Authenticates all ROMs before copying immutable snapshots. Actual symbols
come ONLY from the pinned ELF. This neither opens a card nor grants native
ownership, a completion watchdog, model quality or physical performance.
"""
import os
import pathlib
import re
import subprocess
import sys
import tempfile
from pioneer_q38_manual_ttsync_component_test import pins, snapshot
from compare_pioneer_q38_observed_trace import require

require(len(sys.argv) in (3, 4), 'CONSTANT_Q4 MANUAL_TTSYNC [--pilot]')
require(len(sys.argv) == 3 or sys.argv[3] == '--pilot', 'only explicit partial pilot')
require(not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ), 'no role override')
here = pathlib.Path(__file__).resolve().parent
control, image = map(pathlib.Path, sys.argv[1:3])
mixed = control / 'unchanged-wide/unchanged-twin/unchanged-batched/unchanged-prefetch/unchanged-beta/unchanged-pushed/unchanged-precise/unchanged-global/qualified-math'
snapshots = [snapshot(directory, pins(script)) for directory, script in (
    (mixed, here / 'run_pioneer_q38_mixed_worker_tests.py'),
    (control, here.parent / 'build_qwen38_q4_constant_issue_worker_image.py'),
    (image, here.parent / 'build_qwen38_manual_ttsync_worker.py'))]
legacy_records = [(name + '.bin', size, digest) for name, size, digest in
                  pins(here / 'run_pioneer_q38_resident_gdn_output_operator_service_image_tests.py')]
legacy_records.append(('program.elf', 96148, '44bac0c1061f298ed8ccd2a1eb4780371bc7c80031cb230ad2da27f476202d19'))
legacy_snapshot = snapshot(mixed / 'unchanged-worker', legacy_records)
tool = '/Volumes/HaikuBuildLocal/generated.riscv64/cross-tools-riscv64/bin/riscv64-unknown-haiku-'
flags = re.findall(r'-D[A-Z][A-Z0-9_]*(?:=[0-9]+)?', (here / 'run_pioneer_q38_ffn_persistent_image_tests.sh').read_text())
with tempfile.TemporaryDirectory(prefix='pioneer-q38-manual-ttsync-execution-', dir='/private/tmp') as temporary:
    root = pathlib.Path(temporary)
    images = []
    for index, items in enumerate(snapshots):
        directory = root / ('AUTHENTICATED_ROM_' + str(index))
        directory.mkdir(mode=0o700)
        for name, raw in items.items():
            path = directory / name
            with path.open('xb') as stream:
                stream.write(raw)
            path.chmod(0o400)
        symbols = directory / 'program.symbols'
        symbols.write_bytes(subprocess.check_output([tool + 'nm', '-n', str(directory / 'program.elf')]))
        symbols.chmod(0o400)
        images.append(directory)
    legacy = images[0] / 'unchanged-worker'
    legacy.mkdir(mode=0o700)
    for name, raw in legacy_snapshot.items():
        path = legacy / name
        with path.open('xb') as stream:
            stream.write(raw)
        path.chmod(0o400)
    binary = root / 'worker-test'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-ffp-contract=off', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=address,undefined', *flags, str(here / 'pioneer_q38_manual_ttsync_worker_test.cpp'), '-o', str(binary)], check=True)
    subprocess.run([str(binary), *map(str, images), *sys.argv[3:]], check=True,
                   env={**os.environ, 'ASAN_OPTIONS': 'quarantine_size_mb=32', 'UBSAN_OPTIONS': 'halt_on_error=1'})
print('CLOSED_MANUAL_TTSYNC_' + ('PARTIAL_PILOT' if len(sys.argv) == 4 else 'ACTUAL_WORKER')
      + '_SYNTHETIC_ENDPOINTS_NOT_NATIVE_OR_PHYSICAL_TIMING_OR_MODEL_OR_WORK', flush=True)
