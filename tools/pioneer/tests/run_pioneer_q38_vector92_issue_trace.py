#!/usr/bin/env python3
"""Count issued work in the exact deployed vec92 math worker; offline only."""
import ast
import hashlib
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile

HERE = Path(__file__).resolve().parent
FROZEN = '5a95c1c42983751153df493c2540d9b6501520d8'  # tag92 / tag213 worker and fixture
TARGET = '4b3dc0c96d3888b9c43af683f58678cc91cade7c67e955413bad8c5e16d52870'


def pins(script):
    declarations = [ast.literal_eval(node.value) for node in ast.parse(script.read_text()).body
                    if isinstance(node, ast.Assign) and any(isinstance(target, ast.Name) and target.id == 'PINS'
                                                           for target in node.targets)]
    assert len(declarations) == 1 and declarations[0]
    return declarations[0]


def snapshot(source, destination, records):
    assert source.is_absolute() and source.is_dir() and not source.is_symlink()
    destination.mkdir(mode=0o700)
    for name, size, digest in records:
        path = source / name
        assert path.is_file() and not path.is_symlink()
        data = path.read_bytes()
        assert len(data) == size and hashlib.sha256(data).hexdigest() == digest, name
        (destination / name).write_bytes(data)


def main():
    if len(sys.argv) != 3:
        raise SystemExit('use SDK COMPLETE_PINNED_VECTOR92_WORKER_BUILD')
    assert __debug__ and not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
    sdk, worker = map(Path, sys.argv[1:])
    assert sdk.is_absolute() and worker.is_absolute()
    tool = str(sdk / 'cross-tools-riscv64/bin/riscv64-unknown-haiku-')
    mixed = worker / ('unchanged-wide/unchanged-twin/unchanged-batched/unchanged-prefetch/unchanged-beta/'
                      'unchanged-pushed/unchanged-precise/unchanged-global/qualified-math')
    with tempfile.TemporaryDirectory(prefix='q38-vector92-exact-issues-') as scratch:
        tmp = Path(scratch)
        tree = tmp/'source'
        tree.mkdir(mode=0o700)
        with subprocess.Popen(['git', '-C', HERE.parents[2], 'archive', FROZEN, 'tools/pioneer',
                               'src/add-ons/kernel/drivers/misc'], stdout=subprocess.PIPE) as archive:
            with archive.stdout:
                subprocess.run(['tar', '-x', '-C', tree], stdin=archive.stdout, check=True)
            assert archive.wait() == 0
        tests = tree/'tools/pioneer/tests'
        legacy = [(name+'.bin', size, digest) for name, size, digest in
                  pins(tests/'run_pioneer_q38_resident_gdn_output_operator_service_image_tests.py')]
        legacy.append(('program.elf', 96148, '44bac0c1061f298ed8ccd2a1eb4780371bc7c80031cb230ad2da27f476202d19'))
        flags = re.findall(r'-D[A-Z][A-Z0-9_]*(?:=[0-9]+)?',
                           (tests/'run_pioneer_q38_ffn_persistent_image_tests.sh').read_text())
        vec, prior = tmp/'vector92', tmp/'mixed'
        vector_pins = pins(tests.parent/'build_qwen38_vector_q4_metadata_worker.py')
        assert ('program.elf', 144592, TARGET) in vector_pins
        snapshot(worker, vec, vector_pins)
        snapshot(mixed, prior, pins(tests/'run_pioneer_q38_mixed_worker_tests.py'))
        snapshot(mixed/'unchanged-worker', prior/'unchanged-worker', legacy)
        for directory in (vec, prior):
            symbols = subprocess.check_output([tool+'nm', '-n', directory/'program.elf'])
            (directory/'program.symbols').write_bytes(symbols)
        entries = re.findall(r'^([0-9a-f]+) T q38_compute_once$', (vec/'program.symbols').read_text(), re.M)
        assert len(entries) == 1
        name = 'pioneer_q38_vector92_issue_trace.cpp'
        assert not (tests/name).exists()
        shutil.copyfile(HERE/name, tests/name)
        binary = tmp/'test'
        subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O2', '-g', '-Wall', '-Wextra', '-Werror',
                        '-ffp-contract=off', '-fsanitize=address,undefined', *flags,
                        tests/name, '-o', binary], check=True)
        subprocess.run([binary, prior, vec, entries[0]], check=True,
                       env={**os.environ, 'ASAN_OPTIONS': 'quarantine_size_mb=32', 'UBSAN_OPTIONS': 'halt_on_error=1'})


if __name__ == '__main__':
    main()
