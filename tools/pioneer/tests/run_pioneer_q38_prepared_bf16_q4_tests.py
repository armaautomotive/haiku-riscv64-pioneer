#!/usr/bin/env python3
"""Bounded main/clean arithmetic and RV32 codegen. No device/network access."""
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys

assert __debug__ and not os.environ.get('PYTHONOPTIMIZE') and len(sys.argv) == 3
sdk, out = map(Path, sys.argv[1:])
assert sdk.is_absolute() and sdk.is_dir() and out.is_absolute() and not out.exists() and not out.is_symlink()
assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
here = Path(__file__).resolve().parent
root = here.parents[2]
assert not out.is_relative_to(root)
host = os.environ.get('CXX', 'c++')
cross = str(sdk / 'cross-tools-riscv64/bin/riscv64-unknown-haiku-g++')
nm = str(sdk / 'cross-tools-riscv64/bin/riscv64-unknown-haiku-nm')
unit = Path('tools/pioneer/tests/pioneer_q38_prepared_bf16_q4_test.cpp')
producer = Path('tools/pioneer/blackhole_q38_prepared_bf16_q4_compute.cpp')
regression = Path('tools/pioneer/tests/pioneer_q38_q4_factored_operator_test.cpp')
flags = ['-std=c++17', '-Os', '-Wall', '-Wextra', '-Werror', '-ffreestanding', '-fno-builtin',
         '-fno-tree-loop-distribute-patterns', '-fno-exceptions', '-fno-rtti', '-fno-stack-protector',
         '-fno-pic', '-fno-pie', '-ffp-contract=off', '-ffunction-sections', '-fdata-sections',
         '-fno-unwind-tables', '-fno-asynchronous-unwind-tables', '-msmall-data-limit=0', '-mno-relax',
         '-mabi=ilp32', '-march=rv32i', '-ffixed-s11', '-fomit-frame-pointer', '-fstack-usage',
         '-DPIONEER_Q38_PREPARED_BF16_Q4_BUILD=1', '-DPIONEER_Q38_Q4_FACTORED_BFP8_BUILD=1']
dependencies = {Path(__file__).resolve()}
for cc, path, extra in ((host, unit, []), (host, regression, []), (cross, producer, flags)):
    data = subprocess.check_output([cc, '-std=c++17', *(f for f in extra if f != '-fstack-usage'), '-MM', str(root/path)], text=True)
    for name in data.replace('\\\n', '').split(':', 1)[1].split():
        p = Path(name).resolve()
        assert p.is_relative_to(root)
        dependencies.add(p)
frozen = {p: hashlib.sha256(p.read_bytes()).hexdigest() for p in dependencies}
out.mkdir(mode=0o700)
clean = out/'clean-source'
for p in dependencies:
    target = clean/p.relative_to(root)
    target.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(p, target)
exits, commands = {}, []
def same():
    for p, digest in frozen.items():
        assert hashlib.sha256(p.read_bytes()).hexdigest() == digest
        assert hashlib.sha256((clean/p.relative_to(root)).read_bytes()).hexdigest() == digest
def run(name, args):
    same(); commands.append([name, *map(str, args)])
    with (out/(name+'.stdout')).open('xb') as stdout, (out/(name+'.stderr')).open('xb') as stderr:
        r = subprocess.run(list(map(str, args)), stdout=stdout, stderr=stderr,
                           env=dict(os.environ, ASAN_OPTIONS='quarantine_size_mb=32', UBSAN_OPTIONS='halt_on_error=1'))
    exits[name] = r.returncode
    (out/'actual-exits.json').write_text(json.dumps(exits, sort_keys=True, indent=2)+'\n')
    assert r.returncode == 0 and not (out/(name+'.stderr')).stat().st_size, name
    print('CLOSED_PREPARED_BF16_LOCAL', name, r.returncode, flush=True)
for label, source in (('main', root), ('clean', clean)):
    binary = out/(label+'-test')
    run(label+'-compile', [host, '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror', '-ffp-contract=off',
                           '-fsanitize=address,undefined', source/unit, '-o', binary])
    run(label+'-arithmetic', [binary])
    run(label+'-rv32', [cross, *flags, '-c', source/producer, '-o', out/(label+'.o')])
    undefined = subprocess.check_output([nm, '-u', str(out/(label+'.o'))], text=True).split()
    assert undefined == ['U', 'pioneer_prepared_bf16_q4_fault'], undefined
    frames = [line.split('\t') for line in (out/(label+'.su')).read_text().splitlines()]
    assert frames and all(len(f) == 3 and f[2] == 'static' and int(f[1]) <= 2048 for f in frames)
    run(label+'-regression-compile', [host, '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                                      '-ffp-contract=off', '-fsanitize=address,undefined', source/regression, '-o', out/(label+'-regression')])
    run(label+'-regression', [out/(label+'-regression')])
assert (out/'main-arithmetic.stdout').read_bytes() == (out/'clean-arithmetic.stdout').read_bytes()
assert (out/'main-regression.stdout').read_bytes() == (out/'clean-regression.stdout').read_bytes()
assert (out/'main.o').read_bytes() == (out/'clean.o').read_bytes()
refused = 0
for label, extra in (
        ('missing-build', ['-UPIONEER_Q38_PREPARED_BF16_Q4_BUILD']),
        ('zero-build', ['-UPIONEER_Q38_PREPARED_BF16_Q4_BUILD', '-DPIONEER_Q38_PREPARED_BF16_Q4_BUILD=0']),
        ('kernel', ['-D_KERNEL_MODE']), ('work', ['-DPIONEER_Q38_WORK_INFERENCE=1']),
        ('cpu-reference', ['-DPIONEER_Q38_CPU_VALIDATION=1']), ('native', ['-DPIONEER_Q38_FULL_OFFLOAD_NATIVE=1']),
        ('client', ['-DPIONEER_Q38_FULL_OFFLOAD_CLIENT=1']), ('fast-math', ['-ffast-math'])):
    result = subprocess.run([cross, *(f for f in flags if f != '-fstack-usage'), *extra, '-fsyntax-only', str(root/producer)], capture_output=True, text=True, cwd=out)
    assert result.returncode and ('Exact separate prepared BF16 Q4' in result.stderr or 'Factored-Q4 arithmetic has NO' in result.stderr), label
    refused += 1
same()
(out/'source-sha256.txt').write_text(''.join(frozen[p]+'  '+str(p.relative_to(root))+'\n' for p in sorted(frozen)))
(out/'test-argv.json').write_text(json.dumps(commands, indent=2)+'\n')
(out/'rv32-identity.txt').write_text(hashlib.sha256((out/'main.o').read_bytes()).hexdigest()+'  main.o=clean.o\n')
print('PASS_PREPARED_BF16_QUALIFICATION children'+str(len(exits))+'_stderr0_sources'+str(len(frozen))+
      '_roleRefusals'+str(refused)+'_mainCleanExact_NO_NATIVE_OR_HARDWARE_SPEED_AUTHORITY', flush=True)
