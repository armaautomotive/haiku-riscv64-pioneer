#!/usr/bin/env python3
"""OFFBOARD compact-original-metadata Q4 FPU checks;no card/cache mutation."""
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys

if not __debug__ or os.environ.get('PYTHONOPTIMIZE'):
    raise SystemExit('Checks required')
assert len(sys.argv) == 4
build, original, out = map(Path, sys.argv[1:])
assert build.is_absolute() and build.is_dir() and original.is_absolute() and original.is_file() and not original.is_symlink()
assert out.is_absolute() and not out.exists() and not out.is_symlink()
assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
here = Path(__file__).resolve().parent
out.mkdir(mode=0o700)
exits = {}
commands = []


def run(name, argv):
    commands.append([name, *map(str, argv)])
    with (out / (name + '.stdout')).open('xb') as stdout, (out / (name + '.stderr')).open('xb') as stderr:
        result = subprocess.run(list(map(str, argv)), stdout=stdout, stderr=stderr, check=False)
    exits[name] = result.returncode
    (out / 'actual-exits.json').write_text(json.dumps(exits, sort_keys=True, indent=2) + '\n')
    assert result.returncode == 0 and not (out / (name + '.stderr')).stat().st_size, name
    print('CLOSED_COMPACT_OFFBOARD', name, result.returncode, flush=True)


run('build', [sys.executable, here.parent / 'build_q38_q4_fpu_compact.py', build, out / 'image', '--inspect-candidate'])
compiler = [os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror', '-ffp-contract=off', '-fsanitize=address,undefined']
sources = (
    ('metadata', 'pioneer_q38_q4_fpu_compact_metadata_test.cpp', []),
    ('pipeline', 'pioneer_q38_q4_fpu_compact_pipeline_test.cpp', []),
    ('image', 'pioneer_q38_q4_fpu_compact_image_test.cpp', [out / 'image']),
    ('old-pipeline', 'pioneer_q38_q4_fpu_pipeline_test.cpp', []),
    ('old-image', 'pioneer_q38_q4_fpu_block_image_test.cpp', [out / 'image/unchanged-expanded']),
    ('model', 'pioneer_q38_q4_fpu_compact_model_test.cpp', [out / 'image', original]),
)
dependencies = {Path(__file__).resolve(), here.parent / 'build_q38_q4_fpu_compact.py'}
root = here.parents[2]
for name, source, arguments in sources:
    binary = out / (name + '-test')
    extra = [here.parent / 'PioneerModelWeightCache.cpp'] if name == 'model' else []
    run(name + '-compile', [*compiler, here / source, *extra, '-o', binary])
    run(name, [binary, *arguments])
    for unit in [here / source, *extra]:
        rows = subprocess.check_output([compiler[0], '-std=c++17', '-MM', str(unit)], text=True)
        for path in rows.replace('\\\n', '').split(':', 1)[1].split():
            dependency = Path(path).resolve()
            assert dependency.is_file() and dependency.is_relative_to(root)
            dependencies.add(dependency)
for name, data, extra in (('fraction', out / 'image.stdout', ['--compact']), ('old-fraction', out / 'old-image.stdout', [])):
    run(name, [sys.executable, here / 'pioneer_q38_q4_fpu_pipeline_fraction_test.py', data, *extra])
    dependencies.add(here / 'pioneer_q38_q4_fpu_pipeline_fraction_test.py')
    dependencies.add(here / 'pioneer_native_bfp_arithmetic_test.py')

tool = str(build / 'cross-tools-riscv64/bin/riscv64-unknown-haiku-')
source = here.parent / 'blackhole_q38_q4_fpu_compact.cpp'
forbidden = ('_KERNEL_MODE', 'PIONEER_Q38_FULL_OFFLOAD_NATIVE', 'PIONEER_Q38_FULL_OFFLOAD_CLIENT',
             'PIONEER_Q38_WORK_INFERENCE', 'TT_PIONEER_Q38_WORK_MODE', 'PIONEER_Q38_CPU_VALIDATION',
             'PIONEER_Q38_FULL_MODEL_CPU_REFERENCE', 'PIONEER_DRIVER_BUILD_TAG', 'TT_PIONEER_DRIVER_RELOAD',
             'PIONEER_Q4_FPU_BLOCK_DIRECT', '__FAST_MATH__')
refused = 0
for label, flags in [('missing', []), ('zero', ['-DPIONEER_Q38_COMPACT_Q4_FPU_IMAGE=0']),
                     ('two', ['-DPIONEER_Q38_COMPACT_Q4_FPU_IMAGE=2'])] + [
        (name + str(value), ['-DPIONEER_Q38_COMPACT_Q4_FPU_IMAGE=1', '-D' + name + '=' + str(value)])
        for name in forbidden for value in (0, 1)]:
    result = subprocess.run([tool + 'g++', '-march=rv32i', '-mabi=ilp32', *flags, '-E', str(source)], capture_output=True, text=True)
    assert result.returncode and 'Distinct compact Q4 OFFBOARD RV32 image only' in result.stderr, label
    refused += 1
(out / 'role-refusals.txt').write_text(f'PASS_COMPACT_IMAGE_ROLE_REFUSALS {refused} definedZeroIncluded_NO_NATIVE_OR_WORK\n')

# Include the actual cross-TU/compiler/linker/assembler source closure too.
dependencies.update(here.parent / name for name in ('build_q38_q4_fpu_block.py', 'blackhole_q38_q4_fpu_compact.cpp',
    'blackhole_q38_q4_fpu_compact_start.S', 'blackhole_q38_q4_fpu_compact.ld', 'blackhole_q38_q4_fpu_block.cpp',
    'blackhole_q38_q4_fpu_block_start.S', 'blackhole_q38_q4_fpu_block.ld'))
for unit in ('blackhole_q38_q4_fpu_compact.cpp', 'blackhole_q38_q4_fpu_block.cpp'):
    flags = ['-DPIONEER_Q38_COMPACT_Q4_FPU_IMAGE=1'] if 'compact' in unit else ['-DPIONEER_Q4_FPU_BLOCK_DIRECT=1']
    rows = subprocess.check_output([tool + 'g++', '-march=rv32i', '-mabi=ilp32', *flags, '-MM', str(here.parent / unit)], text=True)
    dependencies.update(Path(p).resolve() for p in rows.replace('\\\n', '').split(':', 1)[1].split())
records = []
for path in sorted(dependencies, key=lambda p: str(p.relative_to(root))):
    assert path.is_file() and path.is_relative_to(root)
    records.append(hashlib.sha256(path.read_bytes()).hexdigest() + '  ' + str(path.relative_to(root)))
(out / 'source-sha256.txt').write_text('\n'.join(records) + '\n')
(out / 'test-argv.json').write_text(json.dumps(commands, indent=2) + '\n')
print('PASS_COMPACT_FPU_COMPLETE_OFFBOARD actualChildren' + str(len(exits)) + '_strictStderr0_roles' + str(refused)
      + '_SOURCE_CLOSURE' + str(len(records)) + ';NOT_FULL_K_NATIVE_COHORT_MODEL_QUALITY_OR_TPS', flush=True)
