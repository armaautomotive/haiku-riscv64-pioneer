#!/usr/bin/env python3
"""OFFBOARD native87 qualification; never opens/stages/runs a physical card."""
import hashlib
import os
import pathlib
import re
import subprocess
import sys

assert __debug__ and not os.environ.get('PYTHONOPTIMIZE') and len(sys.argv) == 4
sdk, original, output = map(pathlib.Path, sys.argv[1:])
assert all(p.is_absolute() for p in (sdk, original, output)) and sdk.is_dir() and original.is_file()
assert not output.exists() and not output.is_symlink()
assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
here = pathlib.Path(__file__).resolve().parent
root = here.parents[2]
assert root not in output.parents
driver = root / 'src/add-ons/kernel/drivers/misc'
pending = [driver / name for name in ('PioneerNativeMixedFpuStreamVariant.h', 'PioneerNativeMixedFpuStreamProtocol.h',
           'PioneerNativeMixedFpuStreamNative.inc', 'pioneer_native_mixed_fpu_stream_driver.cpp',
           'tenstorrent_pci_probe.cpp', 'PioneerPciProfile.cpp')]
pending += [here.parent / name for name in ('PioneerNativeMixedFpuStreamLayout.h', 'PioneerNativeMixedFpuStreamLease.h',
            'pioneer_native_mixed_fpu_stream_export.cpp', 'build_native_mixed_fpu_stream_diagnostic.py', 'PioneerModelWeightCache.cpp')]
pending += [here / name for name in ('pioneer_native_mixed_fpu_stream_contract_test.cpp', 'pioneer_native_mixed_fpu_stream_lease_test.cpp',
            'pioneer_native_mixed_fpu_stream_real_lease_test.cpp', 'pioneer_native_mixed_fpu_stream_export_test.py',
            'pioneer_native_mixed_fpu_stream_native_test.cpp', 'run_pioneer_native_mixed_fpu_stream_native_tests.py',
            'run_pioneer_native_mixed_fpu_stream_tests.py', 'pioneer_dram_sweep_native_lifecycle_test.cpp',
            'pioneer_native_bfp_arithmetic_test.py')]
old_manifest = here.parent / 'benchmarks/qwen38-q8-fpu-stream-offboard-20261104/main/source-sha256.txt'
assert hashlib.sha256(old_manifest.read_bytes()).hexdigest() == '309dfbaab539ea41f7cacf860d3713606e6070bc5ad487b892bdca0e1b887027'
pending.append(old_manifest)
for line in old_manifest.read_text().splitlines():
    digest, name = line.split('  ')
    assert re.fullmatch('[0-9a-f]{64}', digest)
    path = (root / name).resolve()
    path.relative_to(root)
    assert hashlib.sha256(path.read_bytes()).hexdigest() == digest
    pending.append(path)
pins = {}
private_names = {'PioneerNativeMixedFpuStreamData.h', 'PioneerNativeMixedFpuStreamGeneratedImage.h',
                 'PioneerNativeMixedFpuStreamMetadata.h', 'endpoints.h'}
while pending:
    path = pending.pop().resolve()
    name = str(path.relative_to(root))
    if name in pins:
        continue
    data = path.read_bytes()
    pins[name] = hashlib.sha256(data).hexdigest()
    if path.suffix in ('.h', '.cpp', '.inc'):
        for include in re.findall(rb'^\s*#\s*include\s*"([^"\n]+)"', data, re.M):
            include = include.decode('ascii')
            if include in private_names:
                continue
            selected = path.parent / include
            if not selected.is_file():
                candidates = [root / 'headers/private/drivers' / include, root / 'headers/private/kernel' / include,
                              root / 'headers/private/system' / include, root / 'headers/private/shared' / include,
                              root / 'headers/os/drivers' / include, driver / include]
                matches = [candidate for candidate in candidates if candidate.is_file()]
                assert len(matches) == 1, ('Unresolved/ambiguous public quoted include', name, include)
                selected = matches[0]
            pending.append(selected)
output.mkdir(mode=0o700)
commands = []
env = dict(os.environ, ASAN_OPTIONS='quarantine_size_mb=32', UBSAN_OPTIONS='halt_on_error=1')


def run(label, command):
    commands.append(command)
    closed = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, env=env)
    (output / (label + '.log')).write_bytes(closed.stdout)
    (output / (label + '.stderr')).write_bytes(closed.stderr)
    (output / (label + '.exit')).write_text(str(closed.returncode) + '\n')
    if closed.returncode or closed.stderr:
        raise RuntimeError(f'{label} CLOSED{closed.returncode} stderr_bytes{len(closed.stderr)};inspect PRIVATE evidence,NO promotion')
    print('PASS_CLOSED', label, 'exit0 stdout_sha' + hashlib.sha256(closed.stdout).hexdigest(), 'stderr0', flush=True)


common = [os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
          '-ffp-contract=off', '-fsanitize=address,undefined']
run('export-compile', common + [str(here.parent / 'pioneer_native_mixed_fpu_stream_export.cpp'),
    str(here.parent / 'PioneerModelWeightCache.cpp'), '-o', str(output / 'exporter')])
private = output / 'private-export'
private.mkdir(mode=0o700)
run('export', [str(output / 'exporter'), str(original), str(private)])
run('sdk', [sys.executable, str(here.parent / 'build_native_mixed_fpu_stream_diagnostic.py'), str(sdk), str(private), str(original), str(output / 'build')])
run('contract-compile', common + [str(here / 'pioneer_native_mixed_fpu_stream_contract_test.cpp'), '-o', str(output / 'contract')])
run('contract', [str(output / 'contract')])
for kind, name in ((14, 'q6'), (8, 'q8')):
    for trial in ('lease', 'real_lease'):
        label = name + '-' + trial
        binary = output / label
        run(label + '-compile', common + ['-DPIONEER_MIXED_FPU_LEASE_TYPE=' + str(kind), '-I' + str(output / 'build'),
            '-I' + str(private), '-I' + str(here.parent), str(here / ('pioneer_native_mixed_fpu_stream_' + trial + '_test.cpp')), '-o', str(binary)])
        run(label, [str(binary), str(output / 'build' / (name + '-image5120')), str(output / 'build' / (name + '-image6144'))])
run('native', [sys.executable, str(here / 'run_pioneer_native_mixed_fpu_stream_native_tests.py'), str(output / 'build'), str(private), str(output / 'native')])
run('q8-q6-q4-regression', [sys.executable, str(here / 'run_pioneer_q38_q8_fpu_stream_tests.py'), str(sdk), str(original), str(output / 'regression')])
for path, digest in pins.items():
    assert hashlib.sha256((root / path).read_bytes()).hexdigest() == digest
(output / 'source-sha256.txt').write_text(''.join(pins[path] + '  ' + path + '\n' for path in sorted(pins)))
(output / 'test-argv.txt').write_text('\n'.join('\t'.join(command) for command in commands) + '\n')
print(f'PASS OFFBOARD diagnostic87:{len(commands)} strict CLOSED0/stderr0 children,{len(pins)} complete public source hashes,fresh full-original authenticated17-band export/independent Fraction,all4 ISA images/actual SDK driver linked,control unchanged,122 lease faults/1088 authentic returned words/377 native valid+751 refusals,complete Q8/Q6/Q4 regressions;NO physical stage/open/execution/full-model-quality/real-token/speed authority')
