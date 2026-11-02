#!/usr/bin/env python3
"""New pure DATA counter grammar; no SDK, ROM, worker or hardware execution."""
import os
import pathlib
import subprocess
import sys
import tempfile
assert len(sys.argv)==1 and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
here=pathlib.Path(__file__).resolve().parent
with tempfile.TemporaryDirectory(prefix='pioneer-q38-batched-output-data-',dir='/private/tmp') as name:
    for index,flags in enumerate(([],['-DPIONEER_Q38_GLOBAL_CHAIN_PRECISE_ACTIVATION=1'],
                                 ['-DPIONEER_Q38_GLOBAL_CHAIN_PRECISE_ACTIVATION=1','-DPIONEER_Q38_GLOBAL_CHAIN_PUSHED_COMPLETION=1'])):
        binary=pathlib.Path(name)/('test-'+str(index))
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror',
                        '-fsanitize=address,undefined',*flags,str(here/'pioneer_q38_batched_global_output_closure_data_test.cpp'),'-o',str(binary)],check=True)
        subprocess.run([str(binary)],check=True,env={**os.environ,'UBSAN_OPTIONS':'halt_on_error=1'})
