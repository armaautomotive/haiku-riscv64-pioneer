#!/usr/bin/env python3
"""Pure geometry only,not response/drain/arithmetic/native qualification."""
import os
import pathlib
import subprocess
import sys
import tempfile
assert len(sys.argv)==1 and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
here=pathlib.Path(__file__).resolve().parent
subprocess.run([sys.executable,str(here/'run_pioneer_q38_batched_prefetch_plan_tests.py')],check=True)
with tempfile.TemporaryDirectory(prefix='pioneer-q38-twin-batch-plan-',dir='/private/tmp') as temporary:
    binary=pathlib.Path(temporary)/'test'
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror',
                    '-fsanitize=address,undefined',str(here/'pioneer_q38_twin_batch_prefetch_plan_test.cpp'),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True,env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
