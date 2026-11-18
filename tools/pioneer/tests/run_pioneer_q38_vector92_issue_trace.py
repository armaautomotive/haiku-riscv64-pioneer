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


def pins(script, key='PINS'):
    declarations = [ast.literal_eval(node.value) for node in ast.parse(script.read_text()).body
                    if isinstance(node, ast.Assign) and any(isinstance(target, ast.Name) and target.id == key
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


def issue_rows(output):
    rows = []
    bits = []
    for line in output.splitlines():
        if line.startswith('VECTOR92_BITS '):
            fields = line.split()
            assert len(fields) == 6 and fields[3] in ('groups1', 'groups3')
            assert re.fullmatch(r'[0-9a-f]{'+str(int(fields[3][-1])*16*8)+r'}', fields[5])
            bits.append(fields[1:])
            continue
        if not line.startswith('VECTOR92_ISSUE '):
            continue
        fields = line.split()
        assert len(fields) == 23 and fields[5].startswith('RV') and fields[6].startswith('metadataRV')
        # The new C++ branch changes instruction layout. Do not require
        # exact RV counts; all issue/NoC and affine observations must match.
        rows.append(fields[:5]+fields[7:])
    assert len(rows) == len(bits) == 36 and all(row[1:5] == data[:4] for row, data in zip(rows, bits))
    return rows, bits


def main():
    pair = len(sys.argv) == 5 and sys.argv[3] == '--ordinary-arm-regression'
    real_wire = len(sys.argv) == 6 and sys.argv[3] == '--native-q8-wire-real'
    real = real_wire or len(sys.argv) == 6 and sys.argv[3] == '--native-q8-math-real'
    wire = real_wire or len(sys.argv) == 5 and sys.argv[3] == '--native-q8-wire'
    math = real or wire or (len(sys.argv) == 5 and sys.argv[3] in ('--native-q8-math', '--native-q8-math-pilot'))
    if len(sys.argv) != 3 and not (pair or math):
        raise SystemExit('use SDK COMPLETE_PINNED_VECTOR92_WORKER_BUILD '
                         '[--ordinary-arm-regression|--native-q8-math|--native-q8-math-pilot PINNED_NATIVE_Q8_BUILD] '
                         'or SDK COMPLETE_PINNED_VECTOR92_WORKER_BUILD --native-q8-math-real '
                         'PINNED_NATIVE_Q8_BUILD ABSOLUTE_MODEL_GGUF '
                         'or SDK COMPLETE_PINNED_VECTOR92_WORKER_BUILD --native-q8-wire PINNED_WIRE_WORKER '
                         'or SDK COMPLETE_PINNED_VECTOR92_WORKER_BUILD --native-q8-wire-real PINNED_WIRE_WORKER ABSOLUTE_MODEL_GGUF')
    assert __debug__ and not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
    sdk, worker = map(Path, sys.argv[1:3])
    assert sdk.is_absolute() and worker.is_absolute()
    if real:
        model = Path(sys.argv[5])
        assert model.is_absolute()
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
        arms = [vec]
        if pair or math:
            native = tmp/'native'
            records = (pins(HERE.parent/'build_qwen38_twin_native_q8_wire.py', 'WIRE_WORKER_PINS') if wire
                       else pins(HERE.parent/'build_qwen38_twin_native_q8_worker.py'))
            snapshot(Path(sys.argv[4]), native, records)
            arms.append(native)
        for directory in (*arms, prior):
            symbols = subprocess.check_output([tool+'nm', '-n', directory/'program.elf'])
            (directory/'program.symbols').write_bytes(symbols)
        name = 'pioneer_q38_twin_native_q8_math_test.cpp' if math else 'pioneer_q38_vector92_issue_trace.cpp'
        assert not (tests/name).exists()
        shutil.copyfile(HERE/name, tests/name)
        units = [tests/name]
        if math:
            # Test-only observer override, not a ROM/format/owner change.
            source = tests/'pioneer_q38_direct_srcb_image_test.cpp'
            text = source.read_text()
            signature = 'bool Active()const{return memory.at(0x74038)==1;}'
            assert text.count(signature) == 1
            source.write_text(text.replace(signature, 'virtual '+signature))
            flags.extend(('-DPIONEER_Q38_TEST_PREDECODED_Q4=1', '-DPIONEER_Q38_TEST_NATIVE_Q8=1'))
        if real:
            flags.extend(('-DPIONEER_Q38_OFFBOARD_REAL_Q8=1', '-Wno-deprecated-declarations'))
            units.append(tests.parent/'PioneerModelWeightCache.cpp')
        if wire:
            flags.append('-DPIONEER_Q38_OFFBOARD_NATIVE_Q8_WIRE=1')
            shutil.copyfile(HERE.parent/'PioneerQ38TwinNativeQ8Wire.h',tests.parent/'PioneerQ38TwinNativeQ8Wire.h')
        if real_wire:
            for header in ('PioneerQ38TwinQ8ProjectionFixture.h','PioneerQ38NativeQ8FullSource.h'):
                shutil.copyfile(HERE.parent/header,tests.parent/header)
        binary = tmp/'test'
        subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O2', '-g', '-Wall', '-Wextra', '-Werror',
                        '-ffp-contract=off', '-fsanitize=address,undefined', *flags,
                        *units, '-o', binary], check=True)
        runenv = {**os.environ, 'ASAN_OPTIONS': 'quarantine_size_mb=32', 'UBSAN_OPTIONS': 'halt_on_error=1'}
        if math:
            entries = []
            for directory in arms:
                found = re.findall(r'^([0-9a-f]+) T q38_compute_once$', (directory/'program.symbols').read_text(), re.M)
                assert len(found) == 1
                entries.extend(found)
            selection = [model] if real else ['--wire'] if wire else ['--pilot'] if sys.argv[3] == '--native-q8-math-pilot' else []
            subprocess.run([binary, prior, *arms, *entries, *selection], check=True, env=runenv)
            return
        evidence = []
        for directory in arms:
            entries = re.findall(r'^([0-9a-f]+) T q38_compute_once$', (directory/'program.symbols').read_text(), re.M)
            assert len(entries) == 1
            if not pair:
                subprocess.run([binary, prior, directory, entries[0]], check=True, env=runenv)
            else:
                result = subprocess.check_output([binary, prior, directory, entries[0], '--output-bits'], env=runenv, text=True)
                print(directory.name, flush=True)
                print(result, end='', flush=True)
                evidence.append(issue_rows(result))
        if pair:
            assert evidence[0] == evidence[1]
            print('PASS ordinary-arm regression: identical issued work and every output word across both ELFs; '
                  '36 retained jobs per arm, no native Q8 job admission or device timing', flush=True)


if __name__ == '__main__':
    main()
