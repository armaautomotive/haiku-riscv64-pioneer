#!/usr/bin/env python3
"""Pinned complete-ROM functional inspection only;no native/device grant."""
import ast
import hashlib
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile
from compare_pioneer_q38_observed_trace import require, stable_bytes

require(__debug__ and not os.environ.get('PYTHONOPTIMIZE'), 'checks required')
require(len(sys.argv) == 5 and sys.argv[4] in ('--pilot', '--all-shapes', '--controls', '--history'),
        'REPLAY PREFETCH CANDIDATE --pilot|--all-shapes|--controls|--history')
require(not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ), 'no role override')
replay, prefetch, candidate = map(Path, sys.argv[1:4])
here = Path(__file__).resolve().parent
control = replay / 'unchanged-prepared/unchanged-original'
fixture = control / 'unchanged-vector/unchanged-wide/unchanged-twin/unchanged-batched/unchanged-prefetch/unchanged-beta/unchanged-pushed/unchanged-precise/unchanged-global/qualified-math'
tool = '/Volumes/HaikuBuildLocal/generated.riscv64/cross-tools-riscv64/bin/riscv64-unknown-haiku-'


def pins(path, key='PINS'):
    return next(ast.literal_eval(node.value) for node in ast.parse(path.read_text()).body
                if isinstance(node, ast.Assign) and any(isinstance(t, ast.Name) and t.id == key for t in node.targets))


def snapshot(directory, records):
    require(directory.is_absolute() and directory.is_dir() and not directory.is_symlink() and bool(records), 'real ROM directory')
    result = []
    for name, size, digest in records:
        data = stable_bytes(directory / name, size)
        require(hashlib.sha256(data).hexdigest() == digest, 'exact whole ROM ' + name)
        result.append((name, data))
    return result


snapshot(replay, pins(here.parent / 'build_qwen38_two_product_q4_worker.py', 'REPLAY_PINS'))
images = [snapshot(directory, records) for directory, records in (
    (fixture, pins(here / 'run_pioneer_q38_mixed_worker_tests.py')),
    (prefetch, pins(here.parent / 'build_qwen38_instruction_prefetch_worker.py')),
    (candidate, pins(here.parent / 'build_qwen38_q4_block_integer_worker.py', 'INSPECT_PINS')))]
old = dict(images[1])
require(all(data == old[name] for name, data in images[2] if name not in ('service.bin', 'program.elf')),
        'ALL9 other sections unchanged')
require(dict(images[2])['service.bin'][:9888] == old['service.bin'][:9888], 'complete convolution prefix unchanged')
legacy = [(name + '.bin', size, digest) for name, size, digest in pins(here / 'run_pioneer_q38_resident_gdn_output_operator_service_image_tests.py')]
legacy.append(('program.elf', 96148, '44bac0c1061f298ed8ccd2a1eb4780371bc7c80031cb230ad2da27f476202d19'))
legacy = snapshot(fixture / 'unchanged-worker', legacy)
with tempfile.TemporaryDirectory(prefix='pioneer-block-integer-inspect-', dir='/private/tmp') as temporary:
    root = Path(temporary)
    directories = []
    for side, items in enumerate(images):
        target = root / ('EXACT_UNQUALIFIED_ROM_' + str(side))
        target.mkdir(mode=0o700)
        directories.append(target)
        for name, data in items:
            path = target / name
            with path.open('xb') as stream:
                stream.write(data)
            path.chmod(0o400)
        (target / 'program.symbols').write_bytes(subprocess.check_output([tool + 'nm', '-n', str(target / 'program.elf')]))
        rows = [row.split() for row in subprocess.check_output([tool + 'readelf', '-sW', str(target / 'program.elf')], text=True).splitlines()]
        functions = [row for row in rows if len(row) == 8 and row[3] == 'FUNC' and row[6].isdigit() and row[2].isdigit() and int(row[2])]
        require(bool(functions), 'actual ELF FUNC extents')
        (target / 'program.functions').write_text(''.join(row[7] + ' FUNC ' + row[1] + ' ' + row[2] + '\n' for row in functions))
    prior = directories[0] / 'unchanged-worker'
    prior.mkdir(mode=0o700)
    for name, data in legacy:
        path = prior / name
        with path.open('xb') as stream:
            stream.write(data)
        path.chmod(0o400)
    flags = re.findall(r'-D[A-Z][A-Z0-9_]*(?:=[0-9]+)?', (here / 'run_pioneer_q38_ffn_persistent_image_tests.sh').read_text())
    binary = root / 'worker'
    history = sys.argv[4] == '--history'
    source = here / ('pioneer_q38_q4_block_integer_history_test.cpp' if history else 'pioneer_q38_q4_block_integer_worker_test.cpp')
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-ffp-contract=off', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=address,undefined', *flags, str(source), '-o', str(binary)], check=True)
    print('UNQUALIFIED_BLOCK_INTEGER_SAME_READONLY_WHOLE_ROM_PINS_ONLY_NO_BODY_AUDIT_OR_HARDWARE_GRANT', flush=True)
    arguments = [directories[0], directories[2]] if history else [*directories, sys.argv[4]]
    subprocess.run([str(binary), *map(str, arguments)], check=True,
                   env={**os.environ, 'ASAN_OPTIONS': 'quarantine_size_mb=32', 'UBSAN_OPTIONS': 'halt_on_error=1'})
print('PARTIAL_BLOCK_INTEGER_COMPLETE_WORKER_FUNCTIONAL_ONLY_NO_FULL_FAILURE_HISTORY_BODY_NATIVE_MODEL_OR_TPS_QUALIFICATION', flush=True)
