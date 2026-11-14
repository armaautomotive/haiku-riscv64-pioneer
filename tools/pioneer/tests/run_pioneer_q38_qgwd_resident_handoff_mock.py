#!/usr/bin/env python3
"""Compile the current native handoff body against controlled fake endpoints."""
import os
from pathlib import Path
import subprocess
import tempfile

here = Path(__file__).resolve().parent
root = here.parents[2]
native = (root / 'src/add-ons/kernel/drivers/misc/PioneerQ38QgwdResidentNative.inc').read_text()
start = '__attribute__((noinline)) bool\nPioneerQ38TokenCallScope::NextQgwdResidentRequest'
end = '\nstatic __attribute__((noinline)) status_t Q38QgwdResidentSupport'
assert native.count(start) == 1 and native.count(end) == 1
body = start + native.split(start, 1)[1].split(end, 1)[0]
guard_start = '#if defined(PIONEER_Q38_SMALL_RECURRENT_RESIDENT_HANDOFF_INSPECTION)'
guard_end = '\n#endif'
assert native.count(guard_start) == 1
guard = guard_start + native.split(guard_start, 1)[1].split(guard_end, 1)[0] + guard_end + '\n'
fixture = (here / 'pioneer_q38_qgwd_resident_handoff_mock.cpp').read_text()
assert fixture.count('// HANDOFF_BODY') == 1
source = fixture.replace('// HANDOFF_BODY', body)
with tempfile.TemporaryDirectory(prefix='qgwd-resident-handoff-', dir='/private/tmp') as tmp:
    for name, flags in (('qgwd', []), ('small-recurrent', [
            '-DPIONEER_Q38_SMALL_RECURRENT_RESIDENT_HANDOFF_INSPECTION=1',
            '-DPIONEER_Q38_SMALL_RECURRENT_RESIDENT_REBIND_INSPECTION=1'])):
        binary = str(Path(tmp) / name)
        subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                        '-Wno-misleading-indentation', '-fsanitize=address,undefined', '-I', str(here),
                        *flags, '-x', 'c++', '-', '-o', binary], input=source, text=True, check=True)
        subprocess.run([binary], check=True, env=dict(os.environ, ASAN_OPTIONS='quarantine_size_mb=16',
                                                      UBSAN_OPTIONS='halt_on_error=1'))
    base = ['-DPIONEER_Q38_SMALL_RECURRENT_RESIDENT_HANDOFF_INSPECTION=1',
            '-DPIONEER_Q38_SMALL_RECURRENT_RESIDENT_REBIND_INSPECTION=1']
    compiler = [os.environ.get('CXX', 'c++'), '-x', 'c++', '-E', '-']
    assert subprocess.run(compiler + base, input=guard, text=True, capture_output=True).returncode == 0
    for forbidden in ('_KERNEL_MODE', 'PIONEER_DRIVER_BUILD_TAG', 'PIONEER_Q38_QGWD_RESIDENT_NATIVE_146',
                      'TT_PIONEER_Q38_WORK_MODE', 'PIONEER_Q38_WORK_INFERENCE', 'PIONEER_Q38_CPU_VALIDATION',
                      'PIONEER_Q38_FULL_MODEL_CPU_REFERENCE', 'PIONEER_Q38_FULL_OFFLOAD_NATIVE'):
        result = subprocess.run(compiler + base + ['-D' + forbidden + '=1'], input=guard, text=True, capture_output=True)
        assert result.returncode and 'extracted-body inspection only' in result.stderr, forbidden
    result = subprocess.run(compiler + base[:1], input=guard, text=True, capture_output=True)
    assert result.returncode and 'extracted-body inspection only' in result.stderr
    print('PASS small recurrent handoff role refusals; NO DEVICE')
