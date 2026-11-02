#!/usr/bin/env python3
"""HOST partial-FMA arithmetic candidate tests; no driver, image or device."""
import os
import pathlib
import subprocess
import sys
import tempfile

assert len(sys.argv)==1 and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
here=pathlib.Path(__file__).resolve().parent
compiler=os.environ.get('CXX','c++')
guard=[compiler,'-std=c++17','-fsyntax-only',str(here/'pioneer_q38_layer29_wide_reduction_bridge.cpp')]
refusals=[[],['-DPIONEER_Q38_LAYER29_WIDE_REDUCTION_DIAGNOSTIC=0'],['-DPIONEER_Q38_LAYER29_WIDE_REDUCTION_DIAGNOSTIC=2']]
for forbidden in ('_KERNEL_MODE','PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_FULL_MODEL_CPU_REFERENCE',
                  'PIONEER_Q38_WORK_INFERENCE','__FAST_MATH__','PIONEER_Q38_LAYER29_EMITTED_DELTA_DIAGNOSTIC'):
    refusals.extend([['-DPIONEER_Q38_LAYER29_WIDE_REDUCTION_DIAGNOSTIC=1','-D'+forbidden+'='+str(value)] for value in (0,1,2)])
for flags in refusals:
    result=subprocess.run([*guard,*flags],stdout=subprocess.PIPE,stderr=subprocess.PIPE)
    assert result.returncode and b'Separate HOST arithmetic experiment only' in result.stderr,flags
print('PASS HOST candidate isolation/21 missing,wrong,CPU,native,Work,fast-math+old-emitted refusals;NOT native or hardware capability',flush=True)
with tempfile.TemporaryDirectory(prefix='pioneer-q38-wide-candidate-',dir='/private/tmp') as name:
    binary=pathlib.Path(name)/'test'
    subprocess.run([compiler,'-std=c++17','-O1','-g','-ffp-contract=off',
                    '-Wall','-Wextra','-Werror','-fsanitize=address,undefined',
                    '-DPIONEER_Q38_LAYER29_WIDE_REDUCTION_DIAGNOSTIC=1',
                    str(here/'pioneer_q38_wide_reduction_candidate_test.cpp'),
                    str(here/'pioneer_q38_layer29_wide_reduction_bridge.cpp'),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True,env={**os.environ,'UBSAN_OPTIONS':'halt_on_error=1'})
