#!/usr/bin/env python3
"""Complete local arithmetic/returning-component checks;NO Pioneer access."""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

assert __debug__ and not os.environ.get('PYTHONOPTIMIZE') and len(sys.argv) == 3
sdk, out = map(Path, sys.argv[1:])
assert sdk.is_absolute() and sdk.is_dir() and out.is_absolute() and not out.exists() and not out.is_symlink()
assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
here = Path(__file__).resolve().parent
root = here.parents[2]
cc = os.environ.get('CXX', 'c++')
cross = str(sdk / 'cross-tools-riscv64/bin/riscv64-unknown-haiku-g++')
units = {'operator': here / 'pioneer_q38_q4_factored_operator_test.cpp',
         'matrix': here / 'pioneer_q38_factored_q4_matrix_test.cpp',
         'arithmetic-regression': here / 'pioneer_q38_q4_factored_bfp8_test.cpp'}
dependencies = {Path(__file__).resolve(), here / 'pioneer_q38_factored_q4_roles_test.py',
                here.parent / 'build_q38_factored_q4_matrix.py',
                here / 'run_pioneer_q38_q4_factored_tile_tests.sh',
                here / 'pioneer_q38_q4_factored_tile_roles_test.py',
                here.parent / 'build_q38_q4_factored_bfp8.py'}
def collect(compiler, source, flags=()):
    data = subprocess.check_output([compiler, '-std=c++17', *flags, '-MM', str(source)], text=True)
    for name in data.replace('\\\n', '').split(':', 1)[1].split():
        path = Path(name).resolve()
        assert path.is_relative_to(root)
        dependencies.add(path)
for unit in (*units.values(), here / 'pioneer_q38_q4_factored_tile_test.cpp',
             here / 'pioneer_q38_q4_factored_tile_code_test.cpp', here / 'pioneer_q38_q4_fpu_pipeline_test.cpp'):
    collect(cc, unit)
for name, flags in (('blackhole_q38_factored_q4_matrix.cpp',
                     ['-DPIONEER_Q38_FACTORED_Q4_MATRIX_BUILD=1', '-DPIONEER_Q38_Q4_FACTORED_BFP8_BUILD=1']),
                    ('blackhole_q38_q4_factored_tile.cpp',
                     ['-DPIONEER_Q38_Q4_FACTORED_TILE_BUILD=1', '-DPIONEER_Q38_Q4_FACTORED_BFP8_BUILD=1']),
                    ('blackhole_q38_q4_factored_bfp8.cpp', ['-DPIONEER_Q38_Q4_FACTORED_BFP8_BUILD=1'])):
    collect(cross, here.parent / name, ['-march=rv32i', '-mabi=ilp32', *flags])
dependencies.update(here.parent / ('blackhole_q38_factored_q4_matrix' + suffix) for suffix in ('_start.S', '.ld'))
frozen = {p: hashlib.sha256(p.read_bytes()).hexdigest() for p in dependencies}
out.mkdir(mode=0o700)
exits, commands = {}, []
def run(name, argv):
    assert all(hashlib.sha256(p.read_bytes()).hexdigest() == digest for p, digest in frozen.items())
    commands.append([name, *map(str, argv)])
    with (out / (name + '.stdout')).open('xb') as stdout, (out / (name + '.stderr')).open('xb') as stderr:
        result = subprocess.run(list(map(str, argv)), stdout=stdout, stderr=stderr, check=False)
    exits[name] = result.returncode
    (out / 'actual-exits.json').write_text(json.dumps(exits, sort_keys=True, indent=2) + '\n')
    assert result.returncode == 0 and not (out / (name + '.stderr')).stat().st_size, name
    print('CLOSED_FACTORED_Q4_LOCAL', name, result.returncode, flush=True)
env = os.environ.copy()
env.update(ASAN_OPTIONS='quarantine_size_mb=32', UBSAN_OPTIONS='halt_on_error=1')
os.environ.update({k: env[k] for k in ('ASAN_OPTIONS', 'UBSAN_OPTIONS')})
run('build', [sys.executable, here.parent / 'build_q38_factored_q4_matrix.py', sdk, out / 'returning'])
run('roles', [sys.executable, here / 'pioneer_q38_factored_q4_roles_test.py', sdk])
compiler = [cc, '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror', '-ffp-contract=off', '-fsanitize=address,undefined']
for name, unit in units.items():
    binary = out / (name + '-test')
    run(name + '-compile', [*compiler, unit, '-o', binary])
    run(name, [binary, *([out / 'returning'] if name == 'matrix' else [])])
run('default-tile-regression', ['sh', here / 'run_pioneer_q38_q4_factored_tile_tests.sh', sdk, out / 'default-tile'])
assert all(hashlib.sha256(p.read_bytes()).hexdigest() == digest for p, digest in frozen.items())
(out / 'source-sha256.txt').write_text(''.join(frozen[p] + '  ' + str(p.relative_to(root)) + '\n' for p in sorted(frozen)))
(out / 'test-argv.json').write_text(json.dumps(commands, indent=2) + '\n')
print('PASS_FACTORED_Q4_COMPLETE_LOCAL children' + str(len(exits)) + '_stderr0_frozenSources' + str(len(frozen)) +
      '_NO_NATIVE_OWNER_HARDWARE_MODEL_QUALITY_OR_TPS_AUTHORITY', flush=True)
