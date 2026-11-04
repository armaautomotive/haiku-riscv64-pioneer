#!/usr/bin/env python3
"""Header-only learned owner role gates;NO card or private bytes printed."""
import os
import pathlib
import subprocess
import sys
import tempfile
if not __debug__ or len(sys.argv) != 2:
    raise SystemExit('Use PRIVATE_EXPORT_DIRECTORY')
here = pathlib.Path(__file__).resolve().parent
folder = pathlib.Path(tempfile.mkdtemp(prefix='pioneer-learned-roles.'))
source = '#include "' + str(here.parent / 'PioneerNativeModelBandLease.h') + '"\n'
correct = ['-D_KERNEL_MODE', '-DPIONEER_NATIVE_MODEL_BAND_DIAGNOSTIC=1', '-DTT_PIONEER_DRIVER_RELOAD', '-DPIONEER_DRIVER_BUILD_TAG=0x2026110400010080ULL']
cases = [('correct', correct, True), ('unselected', ['-D_KERNEL_MODE'], False),
         ('wrong-role', [flag for flag in correct if 'DIAGNOSTIC' not in flag] + ['-DPIONEER_NATIVE_MODEL_BAND_DIAGNOSTIC=0'], False),
         ('wrong-tag', [flag for flag in correct if 'BUILD_TAG' not in flag] + ['-DPIONEER_DRIVER_BUILD_TAG=0x2026110400010079ULL'], False),
         ('no-reload', [flag for flag in correct if 'DRIVER_RELOAD' not in flag], False),
         ('no-kernel', correct[1:], False), ('work', correct + ['-DPIONEER_Q38_WORK_INFERENCE'], False),
         ('validation', correct + ['-DPIONEER_Q38_CPU_VALIDATION'], False)]
for name, flags, accepted in cases:
    result = subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-DB_DEVICE_OP_CODES_END=10000', '-I' + sys.argv[1],
                             *flags, '-x', 'c++', '-c', '-', '-o', str(folder / (name + '.o'))], input=source, text=True, capture_output=True)
    assert (result.returncode == 0) == accepted, (name, result.stderr)
    if not accepted:
        assert 'Learned-band lease requires' in result.stderr or 'explicit distinct diagnostic80 role' in result.stderr
print('PASS learned owner roles:8 cases;ONLY explicit80+kernel+reload allowed,old tag/wrong flag/Work/validation/missing role refused;NO physical authority')
