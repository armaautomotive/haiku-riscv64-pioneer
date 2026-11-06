#!/usr/bin/env python3
"""Frozen main/clean returning comparison. No network/device/driver access."""
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys

assert __debug__ and not os.environ.get('PYTHONOPTIMIZE') and len(sys.argv) == 5
sdk, private, original, out = map(Path, sys.argv[1:])
assert all(p.is_absolute() for p in (sdk, private, original, out)) and sdk.is_dir() and private.is_dir() and original.is_file()
assert not out.exists() and not out.is_symlink() and not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
here = Path(__file__).resolve().parent; root = here.parents[2]
assert not out.is_relative_to(root)
host = os.environ.get('CXX', 'c++')
cross = str(sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku-g++')
units = {
    'arithmetic': here/'pioneer_q38_prepared_bf16_q4_test.cpp',
    'comparison': here/'pioneer_q38_prepared_bf16_q4_compare_test.cpp',
    'loader': here/'pioneer_q38_prepared_bf16_q4_loader_test.cpp',
    'old-loader': here/'pioneer_q38_q4_fpu_compact_stream_loader_test.cpp',
    'private': here/'pioneer_q38_prepared_bf16_q4_private_test.cpp',
}
dependencies = {Path(__file__).resolve(), here.parent/'build_q38_prepared_bf16_q4_compare.py', here.parent/'build_q38_factored_q4_compare.py'}
private_dependencies = set()
def collect(cc, path, flags=()):
    data = subprocess.check_output([cc, '-std=c++17', *flags, '-MM', str(path)], text=True)
    for name in data.replace('\\\n', '').split(':', 1)[1].split():
        p = Path(name).resolve()
        if p.is_relative_to(root): dependencies.add(p)
        else:
            assert p.is_relative_to(private), p
            private_dependencies.add(p)
for p in units.values(): collect(host, p, ['-I'+str(private)])
collect(host, here.parent/'PioneerModelWeightCache.cpp')
for stem, role in (('blackhole_q38_prepared_bf16_q4_compare', 'PIONEER_Q38_PREPARED_BF16_COMPARE_BUILD'),
                   ('blackhole_q38_factored_q4_compare', 'PIONEER_Q38_FACTORED_Q4_COMPARE_BUILD')):
    collect(cross, here.parent/(stem+'.cpp'), ['-march=rv32i', '-mabi=ilp32', '-D'+role+'=1',
                                              '-DPIONEER_Q38_Q4_FACTORED_BFP8_BUILD=1'])
    dependencies.update(here.parent/(stem+s) for s in ('_start.S', '.ld'))
dependencies.add(here.parent/'blackhole_q38_q4_kernel_clock.cpp')
frozen = {p: hashlib.sha256(p.read_bytes()).hexdigest() for p in dependencies}
private_frozen = {p: hashlib.sha256(p.read_bytes()).hexdigest() for p in private_dependencies}
out.mkdir(mode=0o700); clean = out/'clean-source'
for p in dependencies:
    target = clean/p.relative_to(root); target.parent.mkdir(parents=True, exist_ok=True); shutil.copyfile(p, target)
exits, commands = {}, []
def same():
    for p, digest in frozen.items():
        assert hashlib.sha256(p.read_bytes()).hexdigest() == digest and hashlib.sha256((clean/p.relative_to(root)).read_bytes()).hexdigest() == digest
    assert all(hashlib.sha256(p.read_bytes()).hexdigest() == digest for p, digest in private_frozen.items())
def run(name, args):
    same(); commands.append([name, *map(str, args)])
    with (out/(name+'.stdout')).open('xb') as stdout, (out/(name+'.stderr')).open('xb') as stderr:
        r = subprocess.run(list(map(str, args)), stdout=stdout, stderr=stderr,
                           env=dict(os.environ, ASAN_OPTIONS='quarantine_size_mb=32', UBSAN_OPTIONS='halt_on_error=1'), cwd=out)
    exits[name] = r.returncode
    (out/'actual-exits.json').write_text(json.dumps(exits, sort_keys=True, indent=2)+'\n')
    assert r.returncode == 0 and not (out/(name+'.stderr')).stat().st_size, name
    print('CLOSED_PREPARED_RETURNING_LOCAL', name, r.returncode, flush=True)
for label, source in (('main', root), ('clean', clean)):
    run(label+'-build', [sys.executable, source/'tools/pioneer/build_q38_prepared_bf16_q4_compare.py', sdk, out/(label+'-image')])
    run(label+'-old114-control', [sys.executable, source/'tools/pioneer/build_q38_factored_q4_compare.py', sdk, out/(label+'-old114'), '--configured'])
    assert hashlib.sha256((out/(label+'-old114/text.bin')).read_bytes()).hexdigest() == '43a1b152f0c7e958b907c1eb7127628b5c67aebe64345227dd71f3c8d17de2ed'
    for name, unit in units.items():
        binary = out/(label+'-'+name)
        args = [host, '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror', '-ffp-contract=off',
                '-fsanitize=address,undefined', '-I'+str(private), source/unit.relative_to(root)]
        if name == 'private': args += [source/'tools/pioneer/PioneerModelWeightCache.cpp']
        run(label+'-'+name+'-compile', [*args, '-o', binary])
        run(label+'-'+name, [binary, *([out/(label+'-image')] if name in ('comparison', 'private') else []),
                            *([original] if name == 'private' else [])])
for name in units:
    assert (out/('main-'+name+'.stdout')).read_bytes() == (out/('clean-'+name+'.stdout')).read_bytes()
for name in ('boot.bin', 'text.bin'):
    assert (out/('main-image/'+name)).read_bytes() == (out/('clean-image/'+name)).read_bytes()
base = [cross, '-std=c++17', '-march=rv32i_zicsr', '-mabi=ilp32', '-fsyntax-only', '-fno-exceptions', '-fno-rtti', '-ffreestanding']
refused = []
for label, flags in (
        ('missing', []), ('zero', ['-DPIONEER_Q38_PREPARED_BF16_COMPARE_BUILD=0']),
        ('two', ['-DPIONEER_Q38_PREPARED_BF16_COMPARE_BUILD=2']),
        *[(name, ['-DPIONEER_Q38_PREPARED_BF16_COMPARE_BUILD=1', flag]) for name, flag in (
            ('old', '-DPIONEER_Q38_FACTORED_Q4_COMPARE_BUILD=1'),
            ('codegen', '-DPIONEER_Q38_PREPARED_BF16_Q4_BUILD=1'),
            ('old-configured', '-DPIONEER_Q38_FACTORED_CONFIGURED_COMPUTE=1'),
            ('cached', '-DPIONEER_Q38_CACHED_FPU_MATRIX_IMAGE=1'),
            ('stream', '-DPIONEER_Q38_COMPACT_Q4_FPU_STREAM_IMAGE=1'),
            ('kernel', '-D_KERNEL_MODE'), ('work', '-DPIONEER_Q38_WORK_INFERENCE=1'),
            ('reference', '-DPIONEER_Q38_CPU_VALIDATION=1'), ('native', '-DPIONEER_Q38_FULL_OFFLOAD_NATIVE=1'),
            ('client', '-DPIONEER_Q38_FULL_OFFLOAD_CLIENT=1'), ('fast-math', '-ffast-math'))]):
    p = subprocess.run([*base, '-DPIONEER_Q38_Q4_FACTORED_BFP8_BUILD=1', *flags,
                        str(here.parent/'blackhole_q38_prepared_bf16_q4_compare.cpp')], capture_output=True, text=True, cwd=out)
    assert p.returncode and any(s in p.stderr for s in ('Exact separate prepared BF16 comparison', 'Prepared BF16 comparison cannot alias', 'Factored-Q4 arithmetic has NO')), label
    refused.append('image:'+label)
for flag in ('-DPIONEER_Q38_FACTORED_Q4_COMPARE_BUILD=1', '-DPIONEER_Q38_COMPACT_Q4_TIMING_IMAGE=1',
             '-DPIONEER_Q38_COMPACT_Q4_FPU_STREAM_IMAGE=1', '-DPIONEER_Q4_FPU_COMPACT_STREAM_K=5120'):
    p = subprocess.run([*base, '-DPIONEER_Q38_PREPARED_BF16_COMPARE_BUILD=1', flag,
                        str(here.parent/'blackhole_q38_q4_kernel_clock.cpp')], capture_output=True, text=True, cwd=out)
    assert p.returncode and 'Exact separate prepared BF16 comparison clock role only' in p.stderr
    refused.append('clock:'+flag)
same()
(out/'source-sha256.txt').write_text(''.join(frozen[p]+'  '+str(p.relative_to(root))+'\n' for p in sorted(frozen)))
(out/'private-source-sha256.txt').write_text(''.join(private_frozen[p]+'  '+str(p.relative_to(private))+'\n' for p in sorted(private_frozen)))
(out/'test-argv.json').write_text(json.dumps(commands, indent=2)+'\n')
(out/'role-refusals.json').write_text(json.dumps(refused, indent=2)+'\n')
print('PASS_PREPARED_RETURNING_QUALIFICATION children'+str(len(exits))+'_stderr0_publicSources'+str(len(frozen))+
      '_privateSources'+str(len(private_frozen))+'_roleRefusals'+str(len(refused))+'_mainCleanImageAndWholeStdoutsExact_NO_NATIVE_OR_HARDWARE_SPEED_AUTHORITY', flush=True)
