#!/usr/bin/env python3
"""Qualify OFFBOARD Q6/Q8 operands/code/original bindings;NO GPU or install."""
import hashlib
import os
import pathlib
import re
import subprocess
import sys

assert __debug__ and len(sys.argv) == 4 and not os.environ.get('PYTHONOPTIMIZE')
sdk, original, output = map(pathlib.Path, sys.argv[1:])
assert all(p.is_absolute() for p in (sdk, original, output))
assert sdk.is_dir() and original.is_file() and not output.exists() and not output.is_symlink()
assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
here = pathlib.Path(__file__).resolve().parent
root = here.parents[2]
names = ('PioneerQ38MixedFpuDecode.h', 'blackhole_q38_mixed_fpu_decode.cpp', 'build_q38_mixed_fpu_decode.py')
tests = ('pioneer_q38_mixed_fpu_decode_test.cpp', 'pioneer_q38_mixed_fpu_decode_code_test.cpp',
         'pioneer_q38_mixed_fpu_model_test.cpp', 'pioneer_q38_mixed_fpu_fraction_test.py',
         'run_pioneer_q38_mixed_fpu_decode_tests.sh', 'run_pioneer_q38_mixed_fpu_decode_code_tests.sh',
         'run_pioneer_q38_mixed_fpu_model_tests.sh', 'run_pioneer_q38_mixed_fpu_tests.py')
pending = [here.parent / n for n in names] + [here / n for n in tests]
pending += [here.parent / 'PioneerModelWeightCache.cpp']
pending += [here / n for n in ('run_pioneer_q38_q4_fpu_decode_tests.sh',
                              'pioneer_q38_q4_fpu_decode_test.cpp',
                              'pioneer_q38_q4_fpu_block_test.cpp',
                              'pioneer_q38_q4_fpu_fraction_test.py')]
pins = {}
# Fingerprint every literal local CXX include transitively,including the
# UNCHANGED existing strict RV32 decoder and vendor-derived FMA observer.
while pending:
    path = pending.pop().resolve()
    name = str(path.relative_to(root))
    if name in pins:
        continue
    data = path.read_bytes()
    pins[name] = hashlib.sha256(data).hexdigest()
    if path.suffix in ('.h', '.cpp', '.inc'):
        for included in re.findall(rb'^\s*#\s*include\s*"([^"\n]+)"', data, re.M):
            pending.append(path.parent / included.decode('ascii'))
output.mkdir(mode=0o700)
commands = []
env = dict(os.environ, ASAN_OPTIONS='quarantine_size_mb=32', UBSAN_OPTIONS='halt_on_error=1')
for label, command in (
    ('decode', ['sh', str(here / 'run_pioneer_q38_mixed_fpu_decode_tests.sh')]),
    ('code', ['sh', str(here / 'run_pioneer_q38_mixed_fpu_decode_code_tests.sh'), str(sdk)]),
    ('model', ['sh', str(here / 'run_pioneer_q38_mixed_fpu_model_tests.sh'), str(original)]),
    ('old-q4-regression', ['sh', str(here / 'run_pioneer_q38_q4_fpu_decode_tests.sh')]),
):
    commands.append(command)
    closed = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, env=env)
    (output / (label + '.log')).write_bytes(closed.stdout)
    (output / (label + '.stderr')).write_bytes(closed.stderr)
    (output / (label + '.exit')).write_text(str(closed.returncode) + '\n')
    if closed.returncode or closed.stderr:
        raise RuntimeError(f'{label} CLOSED{closed.returncode} stderr_bytes{len(closed.stderr)};inspect evidence,NO promotion')
    print('PASS_CLOSED', label, 'exit0 stdout_sha' + hashlib.sha256(closed.stdout).hexdigest(), 'stderr0', flush=True)
for path, digest in pins.items():
    assert hashlib.sha256((root / path).read_bytes()).hexdigest() == digest
bad_group = subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-I' + str(here.parent),
                            '-x', 'c++', '-fsyntax-only', '-'],
                           input=b'#include "PioneerQ38MixedFpuDecode.h"\nstruct E{void Op(unsigned);};\nvoid invalid(){E e;PioneerQ38Q6FpuDecode<4>(e);}\n',
                           stdout=subprocess.PIPE, stderr=subprocess.PIPE)
assert bad_group.returncode and b'Original Q6 group must be0..3' in bad_group.stderr and not bad_group.stdout
print('PASS compile refusal:invalid original Q6 group4;ZERO execution', flush=True)
(output / 'source-sha256.txt').write_text(''.join(pins[p] + '  ' + p + '\n' for p in sorted(pins)))
(output / 'test-argv.txt').write_text('\n'.join('\t'.join(c) for c in commands) + '\n')
print('PASS OFFBOARD mixed FPU:4 strict CLOSED0/stderr0 children;Q6/Q8 exact packed-byte/BF16-RNE domain,independent Fraction,actual unchanged strict RV32 MMIO/direct code,15 authenticated original bands/2654208 weights,old Q4 regression;all transitive source hashes unchanged;NO UNPACK/Dst/MOVD2A/GDDR/native ownership/hardware/quality/speed/Work authority')
