#!/usr/bin/env python3
"""OFFBOARD only:SDK driver,actual methods,canonical RV32 lease and real-data checks."""
import hashlib
import os
import pathlib
import re
import subprocess
import sys

assert __debug__ and not os.environ.get('PYTHONOPTIMIZE') and len(sys.argv) == 5
sdk, private, original, output = map(pathlib.Path, sys.argv[1:])
assert output.is_absolute() and not output.exists() and not output.is_symlink()
assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
here = pathlib.Path(__file__).resolve().parent
root = here.parents[2]
output.mkdir(mode=0o700)
sources = [root / path for path in (
    'src/add-ons/kernel/drivers/misc/PioneerNativeCompactQ4StreamProtocol.h',
    'src/add-ons/kernel/drivers/misc/PioneerNativeCompactQ4StreamNative.inc',
    'src/add-ons/kernel/drivers/misc/pioneer_native_compact_q4_stream_driver.cpp',
    'tools/pioneer/PioneerNativeCompactQ4StreamLayout.h', 'tools/pioneer/PioneerNativeCompactQ4StreamLease.h',
    'tools/pioneer/pioneer_native_compact_q4_stream_export.cpp', 'tools/pioneer/build_native_compact_q4_stream_diagnostic.py',
    'tools/pioneer/tests/pioneer_native_compact_q4_stream_contract_test.cpp',
    'tools/pioneer/tests/pioneer_native_compact_q4_stream_lease_test.cpp',
    'tools/pioneer/tests/pioneer_native_compact_q4_stream_real_lease_test.cpp',
    'tools/pioneer/tests/pioneer_native_compact_q4_stream_native_test.cpp',
    'tools/pioneer/tests/pioneer_native_compact_q4_stream_export_test.py',
    'tools/pioneer/tests/run_pioneer_native_compact_q4_stream_native_tests.py',
    'tools/pioneer/tests/run_pioneer_native_compact_q4_stream_tests.py',
    'tools/pioneer/tests/pioneer_dram_sweep_native_lifecycle_test.cpp',
    'tools/pioneer/tests/pioneer_native_bfp_arithmetic_test.py',
    'src/add-ons/kernel/drivers/misc/PioneerPciProfile.cpp',
    'tools/pioneer/build_q38_q4_fpu_compact_stream.py',
    'tools/pioneer/build_q38_q4_fpu_stream.py',
    'tools/pioneer/blackhole_q38_q4_fpu_compact_stream.cpp',
    'tools/pioneer/blackhole_q38_q4_fpu_compact_stream_start.S',
    'tools/pioneer/blackhole_q38_q4_fpu_compact_stream.ld',
    'tools/pioneer/blackhole_q38_q4_fpu_stream.cpp',
    'tools/pioneer/blackhole_q38_q4_fpu_stream_start.S',
    'tools/pioneer/blackhole_q38_q4_fpu_stream.ld')]
# Freeze the complete repository-local quoted-include closure,not just entry files.
queue = list(sources)
while queue:
    source = queue.pop()
    for include in re.findall(r'^\s*#include\s+"([^"]+)"', source.read_text(), re.M):
        for base in (source.parent, here, here.parent, root / 'src/add-ons/kernel/drivers/misc'):
            path = (base / include).resolve()
            if path.is_file() and root in path.parents:
                if path not in sources:
                    sources.append(path)
                    queue.append(path)
                break
sources = sorted(set(sources))
pins = {path: hashlib.sha256(path.read_bytes()).hexdigest() for path in sources}
commands = []
exits = {}
def run(label, command):
    commands.append(command)
    result = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    (output / (label + '.log')).write_bytes(result.stdout)
    (output / (label + '.stderr')).write_bytes(result.stderr)
    exits[label] = result.returncode
    (output / 'actual-child-exits.txt').write_text(''.join(f'{name} {code}\n' for name, code in exits.items()))
    if result.returncode or result.stderr:
        # Private compiler source snippets must not be dumped automatically.
        raise RuntimeError(f'{label} CLOSED {result.returncode},stderr_bytes{len(result.stderr)};inspect PRIVATE output before reporting')
    print(f'PASS_CLOSED {label} exit0 stdout_sha' + hashlib.sha256(result.stdout).hexdigest() + ' stderr0', flush=True)
    return result.stdout.decode()
run('sdk', [sys.executable, str(here.parent / 'build_native_compact_q4_stream_diagnostic.py'), str(sdk), str(private), str(original), str(output / 'build')])
common = [os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror', '-ffp-contract=off', '-fsanitize=address,undefined']
for name in ('contract', 'lease', 'real_lease'):
    run(name + '-compile', common + ['-I' + str(output / 'build'), '-I' + str(private), '-I' + str(here.parent),
        str(here / ('pioneer_native_compact_q4_stream_' + name + '_test.cpp')), '-o', str(output / name)])
    run(name, [str(output / name)] + ([] if name == 'contract' else [str(output / 'build/image5120'),str(output / 'build/image17408')]))
run('native', [sys.executable, str(here / 'run_pioneer_native_compact_q4_stream_native_tests.py'), str(output / 'build'), str(private), str(output / 'native')])
for path, digest in pins.items():
    assert hashlib.sha256(path.read_bytes()).hexdigest() == digest
(output / 'source-sha256.txt').write_text('\n'.join(digest + '  ' + str(path.relative_to(root)) for path, digest in pins.items()) + '\n')
(output / 'test-argv.txt').write_text('\n'.join('\t'.join(c) for c in commands) + '\n')
assert len(exits) == 8 and all(code == 0 for code in exits.values())
print('PASS OFFBOARD diagnostic104 full SDK/contract/lease/real-data/native-method suite;NO hardware/stage/run/retry/reset/model-quality/speed authority')
