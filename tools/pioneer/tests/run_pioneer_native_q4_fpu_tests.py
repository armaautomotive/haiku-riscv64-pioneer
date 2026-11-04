#!/usr/bin/env python3
"""OFFBOARD only:SDK driver,actual methods,canonical RV32 lease and real-data checks."""
import hashlib
import os
import pathlib
import subprocess
import sys

assert __debug__ and not os.environ.get('PYTHONOPTIMIZE') and len(sys.argv) == 4
sdk, private, output = map(pathlib.Path, sys.argv[1:])
assert output.is_absolute() and not output.exists() and not output.is_symlink()
assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
here = pathlib.Path(__file__).resolve().parent
root = here.parents[2]
output.mkdir(mode=0o700)
sources = [root / path for path in (
    'src/add-ons/kernel/drivers/misc/PioneerNativeQ4FpuProtocol.h',
    'src/add-ons/kernel/drivers/misc/PioneerNativeQ4FpuNative.inc',
    'src/add-ons/kernel/drivers/misc/pioneer_native_q4_fpu_driver.cpp',
    'tools/pioneer/PioneerNativeQ4FpuLayout.h', 'tools/pioneer/PioneerNativeQ4FpuLease.h',
    'tools/pioneer/pioneer_native_q4_fpu_export.cpp', 'tools/pioneer/build_native_q4_fpu_diagnostic.py',
    'tools/pioneer/tests/pioneer_native_q4_fpu_contract_test.cpp',
    'tools/pioneer/tests/pioneer_native_q4_fpu_lease_test.cpp',
    'tools/pioneer/tests/pioneer_native_q4_fpu_real_lease_test.cpp',
    'tools/pioneer/tests/pioneer_native_q4_fpu_native_test.cpp',
    'tools/pioneer/tests/pioneer_native_q4_fpu_export_test.py',
    'tools/pioneer/tests/run_pioneer_native_q4_fpu_native_tests.py',
    'tools/pioneer/tests/run_pioneer_native_q4_fpu_tests.py')]
pins = {path: hashlib.sha256(path.read_bytes()).hexdigest() for path in sources}
commands = []
def run(label, command):
    commands.append(command)
    result = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    (output / (label + '.log')).write_bytes(result.stdout)
    (output / (label + '.stderr')).write_bytes(result.stderr)
    if result.returncode or result.stderr:
        # Private compiler source snippets must not be dumped automatically.
        raise RuntimeError(f'{label} CLOSED {result.returncode},stderr_bytes{len(result.stderr)};inspect PRIVATE output before reporting')
    print(f'PASS_CLOSED {label} exit0 stdout_sha' + hashlib.sha256(result.stdout).hexdigest() + ' stderr0', flush=True)
    return result.stdout.decode()
run('sdk', [sys.executable, str(here.parent / 'build_native_q4_fpu_diagnostic.py'), str(sdk), str(private), str(output / 'build')])
common = [os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror', '-ffp-contract=off', '-fsanitize=address,undefined']
for name in ('contract', 'lease', 'real_lease'):
    run(name + '-compile', common + ['-I' + str(output / 'build'), '-I' + str(private), '-I' + str(here.parent),
        str(here / ('pioneer_native_q4_fpu_' + name + '_test.cpp')), '-o', str(output / name)])
    run(name, [str(output / name)] + ([] if name == 'contract' else [str(output / 'build/image')]))
run('native', [sys.executable, str(here / 'run_pioneer_native_q4_fpu_native_tests.py'), str(output / 'build'), str(private), str(output / 'native')])
for path, digest in pins.items():
    assert hashlib.sha256(path.read_bytes()).hexdigest() == digest
(output / 'source-sha256.txt').write_text('\n'.join(digest + '  ' + str(path.relative_to(root)) for path, digest in pins.items()) + '\n')
(output / 'test-argv.txt').write_text('\n'.join('\t'.join(c) for c in commands) + '\n')
print('PASS OFFBOARD diagnostic85 full SDK/contract/lease/real-data/native-method suite;NO hardware/stage/run/retry/reset/model-quality/speed authority')
