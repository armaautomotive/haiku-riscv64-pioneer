#!/usr/bin/env python3
"""Authenticate QGW3 artifacts and check proposed journal DATA; no device IO."""
import os
import pathlib
import runpy
import subprocess
import sys
import tempfile

assert len(sys.argv) == 3 and not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
here = pathlib.Path(__file__).resolve().parent
root, worker = map(pathlib.Path, sys.argv[1:])
assert all(p.is_absolute() and p.is_dir() and not p.is_symlink() for p in (root, worker))
helpers = runpy.run_path(str(here.parent/'generate_qwen38_token_programs.py'))
records, authenticated = helpers['pins'], helpers['authenticated']
# Both complete ELFs, not just section lengths, must match the fixed pins.
sources = [(root, records(here.parent/'build_qwen38_token_root.py', 'PINS_CANCEL_F32_PUSHED')),
           (worker, records(here.parent/'build_qwen38_pushed_worker_image.py', 'PINS'))]
snapshots = [authenticated(directory, pins) for directory, pins in sources]
assert sum(len(snapshot) for snapshot in snapshots) == 15
with tempfile.TemporaryDirectory(prefix='pioneer-q38-pushed-footprint-', dir='/private/tmp') as temporary:
    directory = pathlib.Path(temporary)
    copied = []
    for name, snapshot in zip(('root', 'worker'), snapshots):
        target = directory/name
        target.mkdir()
        for filename, data in snapshot.items():
            (target/filename).write_bytes(data)  # SAME authenticated bytes, never reopen source
        copied.append(target)
    refusals = 0
    for target, (_, pins), snapshot in zip(copied, sources, snapshots):
        for filename, _, _ in pins:
            data = snapshot[filename]
            corrupt = bytearray(data)
            corrupt[len(corrupt)//2] ^= 1
            # Every other privately copied artifact remains valid. These
            # exercise the SAME authentication routine used above, before
            # importing geometry or compiling/running the coverage test.
            for bad in (corrupt, data[:-1]):
                (target/filename).write_bytes(bad)
                try:
                    authenticated(target, pins)
                except ValueError as error:
                    assert filename in str(error), error
                    refusals += 1
                else:
                    raise AssertionError('modified artifact accepted: '+filename)
            (target/filename).write_bytes(data)
    assert refusals == 30
    binary = directory/'test'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=address,undefined', str(here/'pioneer_q38_pushed_footprint_test.cpp'), '-o', str(binary)], check=True)
    subprocess.run([str(binary), *map(str, copied)], check=True,
                   env={**os.environ, 'ASAN_OPTIONS':'quarantine_size_mb=32', 'UBSAN_OPTIONS':'halt_on_error=1'})
    print('PASS proposed QTP3 geometry/15 fixed artifacts including both ELFs/30 corruption+truncation refusals/SAME authenticated snapshots;NO native owner or launch grant')
