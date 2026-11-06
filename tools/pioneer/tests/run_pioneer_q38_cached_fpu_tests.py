#!/usr/bin/env python3
"""Execute the returning cached-FPU/BF16 services OFFBOARD, not inference."""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

assert __debug__ and not os.environ.get('PYTHONOPTIMIZE') and len(sys.argv) == 5
sdk, original, private, out = map(Path, sys.argv[1:])
assert all(p.is_absolute() for p in (sdk, original, private, out))
assert sdk.is_dir() and original.is_file() and not original.is_symlink() and private.is_dir()
assert not out.exists() and not out.is_symlink()
assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
here = Path(__file__).resolve().parent
root = here.parents[2]
pins = {
    'PioneerNativeCompactQ4StreamData.h': '4db508f0cecda10062b7373d43f4b6e7308ad8cdcff03c361759ac0523a896b1',
    'PioneerNativeCompactQ4StreamMetadata.h': 'c856c1747045a50c43e5b1f47a7e75999526bbc61960eed075246423392872de',
}
for name, digest in pins.items():
    assert not (private/name).is_symlink() and hashlib.sha256((private/name).read_bytes()).hexdigest() == digest
out.mkdir(mode=0o700)
exits = {}
commands = []
dependencies = {Path(__file__).resolve(), here/'pioneer_q38_cached_bf16_fraction_test.py',
                here.parent/'build_q38_cached_fpu_services.py'}
tool = str(sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku-')
compiler = [os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
            '-ffp-contract=off', '-fsanitize=address,undefined']

def collect(compiler_name, unit, flags=()):
    data = subprocess.check_output([str(compiler_name), '-std=c++17', *flags, '-MM', str(unit)], text=True)
    for name in data.replace('\\\n', '').split(':', 1)[1].split():
        path = Path(name).resolve()
        if path.is_relative_to(root):
            dependencies.add(path)
        else:
            assert path.parent == private and path.name in pins

units = {}
for name in ('contract', 'image', 'fault', 'warm', 'original'):
    unit = here/('pioneer_q38_cached_fpu_'+name+'_test.cpp')
    extra = [here.parent/'PioneerModelWeightCache.cpp'] if name == 'original' else []
    flags = ['-I'+str(private)] if name == 'original' else []
    units[name] = (unit, extra, flags)
    for source in (unit, *extra):
        collect(compiler[0], source, flags)
for name, role in (('cached_fpu_matrix', 'PIONEER_Q38_CACHED_FPU_MATRIX_IMAGE'),
                   ('bf16_producer', 'PIONEER_Q38_BF16_PRODUCER_IMAGE')):
    collect(tool+'g++', here.parent/('blackhole_q38_'+name+'.cpp'),
            ['-march=rv32i', '-mabi=ilp32', '-D'+role+'=1'])
    dependencies.update(here.parent/('blackhole_q38_'+name+suffix) for suffix in ('_start.S', '.ld'))
frozen = {p: hashlib.sha256(p.read_bytes()).hexdigest() for p in dependencies}

def run(name, argv):
    commands.append([name, *map(str, argv)])
    with (out/(name+'.stdout')).open('xb') as stdout, (out/(name+'.stderr')).open('xb') as stderr:
        result = subprocess.run(list(map(str, argv)), stdout=stdout, stderr=stderr, check=False)
    exits[name] = result.returncode
    (out/'actual-exits.json').write_text(json.dumps(exits, sort_keys=True, indent=2)+'\n')
    assert result.returncode == 0 and not (out/(name+'.stderr')).stat().st_size, name
    print('CLOSED_CACHED_FPU_OFFBOARD', name, result.returncode, flush=True)

run('build', [sys.executable, here.parent/'build_q38_cached_fpu_services.py', sdk, out/'services'])
producer = out/'services/bf16_producer'
matrix = out/'services/cached_fpu_matrix'
for name, (unit, extra, flags) in units.items():
    binary = out/(name+'-test')
    run(name+'-compile', [*compiler, *flags, unit, *extra, '-o', binary])
    args = [] if name == 'contract' else [producer, matrix, *([original] if name == 'original' else [])]
    run(name, [binary, *args])
run('fraction', [sys.executable, here/'pioneer_q38_cached_bf16_fraction_test.py', out/'image.stdout'])
assert all(hashlib.sha256(p.read_bytes()).hexdigest() == digest for p, digest in frozen.items())
assert all(hashlib.sha256((private/name).read_bytes()).hexdigest() == digest for name, digest in pins.items())
(out/'source-sha256.txt').write_text(''.join(frozen[p]+'  '+str(p.relative_to(root))+'\n' for p in sorted(frozen)))
(out/'private-source-sha256.json').write_text(json.dumps(pins, sort_keys=True, indent=2)+'\n')
(out/'test-argv.json').write_text(json.dumps(commands, indent=2)+'\n')
print('PASS_CACHED_FPU_COMPLETE_OFFBOARD children'+str(len(exits))+'_stderr0_frozenSources'+str(len(frozen))+
      '_NO_NATIVE_PRODUCER_LIFETIME_FULL_MODEL_HARDWARE_OR_TPS_AUTHORITY', flush=True)
