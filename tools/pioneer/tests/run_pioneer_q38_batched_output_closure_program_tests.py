#!/usr/bin/env python3
"""Authenticated corrected ROOT package DATA; never native execution/device IO."""
import hashlib
import os
import pathlib
import re
import runpy
import subprocess
import sys
import tempfile

assert len(sys.argv) == 3 and not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
root, worker = map(pathlib.Path, sys.argv[1:])
assert all(p.is_absolute() and p.is_dir() and not p.is_symlink() for p in (root, worker))
here = pathlib.Path(__file__).resolve().parent
saved = sys.argv[:]
try:
    sys.argv = [str(here / 'run_pioneer_q38_twin_batch_prefetch_program_tests.py'), str(root / 'unchanged-batched'), str(worker)]
    old = runpy.run_path(sys.argv[0], run_name='__main__')
finally:
    sys.argv = saved
sys.stdout.flush()
generator = here.parent / 'generate_qwen38_token_programs.py'
helpers = runpy.run_path(str(generator))
inputs = ((root, helpers['pins'](here.parent / 'build_qwen38_batched_output_closure_root.py', 'PINS')),
          (worker, helpers['pins'](here.parent / 'build_qwen38_token_twin_batch_prefetch_worker.py', 'PINS')))
for directory, records in inputs:
    helpers['authenticated'](directory, records)
committed = here.parent / 'PioneerQ38TokenBatchedOutputClosurePrograms.h'
assert hashlib.sha256(committed.read_bytes()).hexdigest() == '1e844aca55fd4920a278fcd702a64b324fe13ea2c29e52d4cd75841e42f06891'
primary = 'PIONEER_Q38_TOKEN_BATCHED_OUTPUT_CLOSURE_PROGRAMS'
with tempfile.TemporaryDirectory(prefix='pioneer-q38-output-closure-data-', dir='/private/tmp') as temporary:
    out = pathlib.Path(temporary)
    header = out / 'new-data.h'
    subprocess.run([sys.executable, str(generator), str(root), str(worker), str(header), '--batched-output-closure'], check=True)
    assert header.read_bytes() == committed.read_bytes()
    common = [os.environ.get('CXX', 'c++'), '-std=c++17', '-Wall', '-Wextra', '-Werror', '-I' + str(here.parent)]
    binary = out / 'test'
    subprocess.run([*common, '-O1', '-g', '-fsanitize=address,undefined', '-D' + primary + '=1',
                    '-DPIONEER_Q38_PROGRAM_DATA_TEST_BATCHED_OUTPUT_CLOSURE=1',
                    '-DPIONEER_Q38_PUSHED_PROGRAMS_HEADER="' + str(header) + '"',
                    str(here / 'pioneer_q38_pushed_programs_test.cpp'), '-o', str(binary)], check=True)
    subprocess.run([str(binary), str(root), str(worker)], check=True,
                   env={**os.environ, 'ASAN_OPTIONS': 'quarantine_size_mb=32', 'UBSAN_OPTIONS': 'halt_on_error=1'})
    corruptions = 0
    for directory, records in inputs:
        for name, _, _ in records:
            case = out / ('bad-' + str(corruptions))
            case.mkdir()
            for file, _, _ in records:
                (case / file).symlink_to(directory / file)
            target = case / name
            target.unlink()  # Only the exact just-created fixture symlink.
            original = (directory / name).read_bytes()
            damaged = bytearray(original)
            damaged[len(damaged) // 2] ^= 1
            for data in (damaged, original[:-1]):
                target.write_bytes(data)
                output = out / ('NO_OUTPUT-' + str(corruptions))
                result = subprocess.run([sys.executable, str(generator), str(case if directory == root else root),
                                         str(case if directory == worker else worker), str(output), '--batched-output-closure'],
                                        capture_output=True)
                assert result.returncode == 2 and name.encode() in result.stderr and not output.exists(), result.stderr
                corruptions += 1
    assert corruptions == 32
    wrong = 0
    for r, w, mode in ((root / 'unchanged-batched', worker, '--batched-output-closure'),
                       (root, worker / 'unchanged-batched', '--batched-output-closure'),
                       (root, worker, '--twin-batch-prefetch'), (root, worker, '--batched-prefetch')):
        output = out / ('wrong-family-' + str(wrong))
        result = subprocess.run([sys.executable, str(generator), str(r), str(w), str(output), mode], capture_output=True)
        assert result.returncode == 2 and not output.exists(), result.stderr
        wrong += 1
    role = here.parent / 'PioneerQ38TokenBatchedOutputClosureProgramsRole.h'
    boundary = here.parent / 'PioneerQ38TokenBatchedOutputClosureDataBoundary.h'
    include = lambda p: '#include "' + str(p) + '"\n'
    positive = negative = 0

    def syntax(values, okay, body=None, selection=1):
        global positive, negative
        source = ''.join('#define ' + key + ' ' + value + '\n' for key, value in sorted(values.items()))
        source += body if body is not None else include(role) + include(role) + \
            f'static_assert(PioneerQ38TokenBatchedOutputClosureDataSelection=={selection});\n'
        result = subprocess.run([*common, '-fsyntax-only', '-x', 'c++', '-'], input=source, text=True, capture_output=True)
        assert (result.returncode == 0) == okay, (values, body, result.stderr)
        if okay:
            positive += 1
        else:
            assert 'error:' in result.stderr
            negative += 1

    base = {primary: '1'}
    syntax({}, True, selection=0)
    syntax(base, True)
    syntax(base, True, include(committed) + include(committed) +
           'static_assert(PioneerQ38TokenBatchedOutputClosurePrograms::BatchedOutputClosure==1);\n')
    for value in (None, '0', '2'):
        syntax({} if value is None else {primary: value}, False, include(committed))
    foreign = set(re.findall(r'defined\(([A-Z_][A-Z0-9_]*)\)', boundary.read_text())) - {primary, 'PIONEER_Q38_TOKEN_BATCHED_OUTPUT_CLOSURE_PROGRAMS_H'}
    for key in sorted(foreign):
        for value in ('0', '1'):
            syntax({**base, key: value}, False)
            syntax(base, False, include(role) + '#define ' + key + ' ' + value + '\n' + include(role))
    for value in ('0', '1'):
        syntax({**base, 'PIONEER_Q38_TOKEN_BATCHED_OUTPUT_CLOSURE_PROGRAMS_H': value}, False,
               include(committed) + 'static_assert(PioneerQ38TokenBatchedOutputClosurePrograms::BatchedOutputClosure==1);\n')
    syntax(base, False, include(here.parent / 'PioneerQ38TokenSelectedPrograms.h'))
    syntax({}, False, include(role) + '#define ' + primary + ' 1\n' + include(role), selection=0)
    syntax(base, False, include(role) + '#undef ' + primary + '\n' + include(role))
    for name in ('PioneerQ38TokenTwinBatchPrefetchPrograms.h', 'PioneerQ38TokenBatchedPrefetchPrograms.h',
                 'PioneerQ38TokenPrefetchPrograms.h', 'PioneerQ38TokenFlatMatrixPrograms.h', 'PioneerQ38TokenPrograms.h'):
        syntax(base, False, include(here.parent / name))
        syntax(base, False, include(committed) + include(here.parent / name))
    print(f'PASS corrected-output14-section DATA/16 authenticated artifacts/{corruptions} corrupt+truncated/{wrong} wrong-family/'
          f'{positive} syntax positives/{negative} missing,zero,two,late,old-array,native,BUILD,USER,CPU+Work refusals;'
          'ALL unchanged twin+ancestors FIRST/ALL120 exact code words in unchanged journal geometry;'
          'NO native lifecycle,model,physical,Work or TPS grant')
