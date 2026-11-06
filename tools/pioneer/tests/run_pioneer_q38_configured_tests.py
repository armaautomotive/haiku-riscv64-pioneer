#!/usr/bin/env python3
"""Frozen local qualification of ONE configuration-lifetime candidate;no device."""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

assert __debug__ and len(sys.argv) == 3 and not os.environ.get('PYTHONOPTIMIZE')
sdk, out = map(Path, sys.argv[1:]); here = Path(__file__).resolve().parent; root = here.parents[2]
assert sdk.is_absolute() and sdk.is_dir() and out.is_absolute() and not out.exists() and not out.is_symlink()
assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
cc = os.environ.get('CXX', 'c++'); cross = str(sdk / 'cross-tools-riscv64/bin/riscv64-unknown-haiku-g++')
builder = here.parent / 'build_q38_factored_q4_compare.py'
units = ['pioneer_q38_q4_configured_operator_test.cpp', 'pioneer_q38_configured_compare_test.cpp']
dependencies = {Path(__file__).resolve(), builder, here / 'pioneer_q38_configured_roles_test.py'}
def collect(compiler, source, extra=()):
    data = subprocess.check_output([compiler, '-std=c++17', *extra, '-MM', str(source)], text=True)
    for name in data.replace('\\\n', '').split(':', 1)[1].split():
        path = Path(name).resolve(); assert path.is_relative_to(root); dependencies.add(path)
for name in units: collect(cc, here / name)
for configured in (False, True):
    flags = ['-march=rv32i', '-mabi=ilp32', '-DPIONEER_Q38_FACTORED_Q4_COMPARE_BUILD=1',
             '-DPIONEER_Q38_Q4_FACTORED_BFP8_BUILD=1']
    if configured: flags += ['-DPIONEER_Q38_FACTORED_CONFIGURED_COMPUTE=1']
    collect(cross, here.parent / 'blackhole_q38_factored_q4_compare.cpp', flags)
collect(cross, here.parent / 'blackhole_q38_q4_kernel_clock.cpp',
        ['-march=rv32i_zicsr', '-mabi=ilp32', '-DPIONEER_Q38_FACTORED_Q4_COMPARE_BUILD=1'])
dependencies.update(here.parent / ('blackhole_q38_factored_q4_compare' + s) for s in ('_start.S', '.ld'))
frozen = {p: hashlib.sha256(p.read_bytes()).hexdigest() for p in dependencies}
out.mkdir(mode=0o700); exits = {}; commands = []
def stable(): return all(hashlib.sha256(p.read_bytes()).hexdigest() == value for p, value in frozen.items())
def run(name, argv):
    assert stable(); commands.append([name, *map(str, argv)])
    with (out / (name + '.stdout')).open('xb') as stdout, (out / (name + '.stderr')).open('xb') as stderr:
        result = subprocess.run(list(map(str, argv)), stdout=stdout, stderr=stderr)
    exits[name] = result.returncode
    (out / 'actual-exits.json').write_text(json.dumps(exits, sort_keys=True, indent=2) + '\n')
    assert result.returncode == 0 and not (out / (name + '.stderr')).stat().st_size, name
    print('CLOSED_CONFIGURED_LOCAL', name, result.returncode, flush=True)
run('default-build', [sys.executable, builder, sdk, out / 'default'])
run('configured-build', [sys.executable, builder, sdk, out / 'configured', '--configured'])
assert hashlib.sha256((out / 'default/text.bin').read_bytes()).hexdigest() == '6542780a82026f4a3a7f410468bb1081a7143991aeb202d39f379a67153dcedd'
assert hashlib.sha256((out / 'configured/text.bin').read_bytes()).hexdigest() == '43a1b152f0c7e958b907c1eb7127628b5c67aebe64345227dd71f3c8d17de2ed'
assert (out / 'default/boot.bin').read_bytes() == (out / 'configured/boot.bin').read_bytes()
run('roles', [sys.executable, here / 'pioneer_q38_configured_roles_test.py', sdk])
os.environ.update(ASAN_OPTIONS='quarantine_size_mb=32', UBSAN_OPTIONS='halt_on_error=1')
for name, source in zip(('operator', 'compiled'), units):
    binary = out / (name + '-test')
    run(name + '-compile', [cc, '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                           '-ffp-contract=off', '-fsanitize=address,undefined', here / source, '-o', binary])
    args = [out / 'default', out / 'configured', '12108'] if name == 'compiled' else []
    run(name, [binary, *args])
assert stable()
(out / 'source-sha256.txt').write_text(''.join(frozen[p] + '  ' + str(p.relative_to(root)) + '\n' for p in sorted(frozen)))
(out / 'test-argv.json').write_text(json.dumps(commands, indent=2) + '\n')
print('PASS_CONFIGURED_COMPONENT_LOCAL children' + str(len(exits)) + '_stderr0_frozenSources' + str(len(frozen)) +
      '_DEFAULT113_UNCHANGED_NO_NATIVE_INTEGRATION_HARDWARE_SPEED_OR_MODEL_TPS', flush=True)
