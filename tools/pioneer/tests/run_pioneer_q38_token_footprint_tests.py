#!/usr/bin/env python3
"""Authenticate complete token/QGW1 DATA;verify every code and snapshot word."""
import ast
import hashlib
import os
import pathlib
import subprocess
import sys
import tempfile

assert len(sys.argv) == 4 and sys.argv[3] in ('--cancel-boundary', '--no-cancel', '--precise-activation')
assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
here = pathlib.Path(__file__).resolve().parent
root, worker = map(pathlib.Path, sys.argv[1:3])
assert root.is_absolute() and worker.is_absolute()
mode = sys.argv[3]
precise = mode == '--precise-activation'
generator = here.parent/'generate_qwen38_token_programs.py'
def records(path, key):
    tree = ast.parse(path.read_text())
    return next(ast.literal_eval(n.value) for n in tree.body if isinstance(n, ast.Assign)
                and any(isinstance(t, ast.Name) and t.id == key for t in n.targets))
inputs = [(root, records(here.parent/'build_qwen38_token_root.py', 'PINS_CANCEL_F32_PRECISE' if precise else 'PINS_CANCEL_F32' if mode == '--cancel-boundary' else 'PINS_F32')),
          (worker, records(here/'run_pioneer_q38_precise_worker_tests.py' if precise else here.parent/'build_qwen38_global_worker_image.py', 'PINS'))]
with tempfile.TemporaryDirectory(prefix='pioneer-token-footprint-', dir='/private/tmp') as tmp:
    d = pathlib.Path(tmp)
    generated = d/'programs.h'
    binary = d/'test'
    subprocess.run([sys.executable, str(generator), str(root), str(worker), str(generated), mode], check=True)
    digest = hashlib.sha256(generated.read_bytes()).hexdigest()
    if mode != '--no-cancel':
        committed = here.parent/('PioneerQ38TokenPrecisePrograms.h' if precise else 'PioneerQ38TokenPrograms.h')
        assert generated.read_bytes() == committed.read_bytes(), 'committed selected DATA must reproduce exactly'
    # Keep every other byte source valid;only the named privately copied
    # artifact is corrupt. A refusal of some unrelated missing input is NOT
    # counted as proof that the selected complete ELF/section was checked.
    refusals = 0
    for directory, pins in inputs:
        for name, size, expected in pins:
            source = (directory/name).read_bytes()
            assert len(source) == size and hashlib.sha256(source).hexdigest() == expected
            case = d/f'bad-{refusals}'
            case.mkdir()
            for path in directory.iterdir():
                if path.is_file():
                    (case/path.name).symlink_to(path)
            target = case/name
            target.unlink()
            corrupt = bytearray(source)
            corrupt[len(corrupt)//2] ^= 1
            target.write_bytes(corrupt)
            output = d/f'NO_OUTPUT-{refusals}'
            result = subprocess.run([sys.executable, str(generator), str(case if directory == root else root),
                                     str(case if directory == worker else worker), str(output), mode], capture_output=True)
            assert result.returncode == 2 and name.encode() in result.stderr and not output.exists(), result.stderr
            refusals += 1
    artifacts = sum(len(pins) for directory, pins in inputs)
    assert refusals == artifacts == (15 if precise else 14)
    result = subprocess.run([sys.executable, str(generator), str(root), str(worker), str(generated), mode], capture_output=True)
    assert result.returncode == 2 and hashlib.sha256(generated.read_bytes()).hexdigest() == digest
    refusals += 1
    for role in ('PIONEER_Q38_WORK_INFERENCE', 'TT_PIONEER_Q38_WORK_MODE'):
        output = d/f'NO_ROLE-{role}'
        result = subprocess.run([sys.executable, str(generator), str(root), str(worker), str(output), mode], capture_output=True,
                                env={**os.environ, role:'0'})
        assert result.returncode == 2 and b'role overrides' in result.stderr and not output.exists()
        refusals += 1
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=address,undefined', '-I'+str(here.parent),
                    *(['-DPIONEER_Q38_TOKEN_PRECISE_PROGRAMS=1'] if precise else []),
                    '-DPIONEER_Q38_TOKEN_PROGRAMS_HEADER="'+str(generated)+'"',
                    str(here/'pioneer_q38_token_footprint_test.cpp'), '-o', str(binary)], check=True)
    subprocess.run([str(binary), str(root), str(worker), str(int(mode != '--no-cancel'))], check=True,
                   env={**os.environ, 'UBSAN_OPTIONS':'halt_on_error=1', 'ASAN_OPTIONS':'quarantine_size_mb=32'})
    print(f'PASS full-token immutable generator/complete{artifacts}artifact auth/SAME-open snapshots/exact reproduction/{refusals} corruption+overwrite+role refusals;DATA ONLY/no device or native permission')
