#!/usr/bin/env python3
"""Independent DEBUG RMS reference and actual header isolation gates."""
import os
import pathlib
import subprocess
import tempfile

here = pathlib.Path(__file__).resolve().parent
header = (here.parent / 'PioneerQ38LearnedRmsValidationReference.h').read_text()
guard = '#if ' + header.split('#if ', 1)[1].split('#endif', 1)[0] + '#endif\n'
valid = ['-DPIONEER_Q38_CPU_VALIDATION=1', '-DPIONEER_Q38_LEARNED_RMS_VALIDATION_REFERENCE=1']
invalid = [[], valid[:1], valid[1:],
           [valid[0], '-DPIONEER_Q38_LEARNED_RMS_VALIDATION_REFERENCE=0'],
           [valid[0], '-DPIONEER_Q38_LEARNED_RMS_VALIDATION_REFERENCE=2'],
           [valid[1], '-DPIONEER_Q38_CPU_VALIDATION=0'],
           [valid[1], '-DPIONEER_Q38_CPU_VALIDATION=2']]
invalid += [valid + ['-D' + role + '=1'] for role in ('PIONEER_Q38_WORK_INFERENCE', 'PIONEER_Q38_NATIVE_Q8_LAYOUT', '_KERNEL_MODE')]
clean = {key: value for key, value in os.environ.items() if not key.startswith('PIONEER_')}
for flags in invalid:
    result = subprocess.run([os.environ.get('CXX', 'c++'), '-E', '-x', 'c++', *flags, '-'], input=guard,
                            capture_output=True, text=True, env=clean)
    assert result.returncode != 0 and 'separate DEBUG ONLY' in result.stderr
result = subprocess.run([os.environ.get('CXX', 'c++'), '-E', '-x', 'c++', *valid, '-'], input=guard,
                        capture_output=True, text=True, env=clean)
assert result.returncode == 0
with tempfile.TemporaryDirectory(prefix='pioneer-rms-debug-reference-') as temporary:
    binary = pathlib.Path(temporary) / 'test'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-ffp-contract=off', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=address,undefined', str(here / 'pioneer_q38_learned_rms_validation_reference_test.cpp'), '-o', str(binary)],
                   check=True, env=clean)
    subprocess.run([str(binary)], check=True,
                   env={**clean, 'ASAN_OPTIONS': 'quarantine_size_mb=32', 'UBSAN_OPTIONS': 'halt_on_error=1'})
print(f'PASS actual independent RMS reference header DEBUG isolation gates{len(invalid)};NO Work/kernel/native-layout fallback')
