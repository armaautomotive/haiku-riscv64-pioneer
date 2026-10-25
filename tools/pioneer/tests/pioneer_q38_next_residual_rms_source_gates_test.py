#!/usr/bin/env python3
"""Keep original-source observers separate from every native63 flag spelling."""
import os
import pathlib
import subprocess

here = pathlib.Path(__file__).resolve().parent
source = here/'pioneer_q38_decoder_original_source.cpp'
cxx = os.environ.get('CXX', 'c++')
checks = 0
for value in ('0', '1', '2', '-1'):
    for extra in ((), ('PIONEER_Q38_NEXT_GDN_OUTPUT_NATIVE=1',), ('PIONEER_Q38_CPU_VALIDATION=1',),
                  ('PIONEER_Q38_WORK_INFERENCE=1',), ('_KERNEL_MODE=1',)):
        result = subprocess.run([cxx, '-std=c++17', '-E', '-DPIONEER_Q38_NEXT_RESIDUAL_RMS_NATIVE='+value,
                                 *('-D'+flag for flag in extra), str(source)], capture_output=True, text=True, timeout=30)
        assert result.returncode != 0 and 'Original residual CPU observer is separate from native63' in result.stderr
        checks += 1
print(f'PASS {checks} native63 defined-even-zero/mixed-role original-observer refusals;NO native/device access')
