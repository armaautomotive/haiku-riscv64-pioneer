#!/usr/bin/env python3
"""QGC1 canonical metadata/job factory DATA proof only."""
import os
import pathlib
import subprocess
import tempfile
here=pathlib.Path(__file__).resolve().parent
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
subprocess.run(['python3',str(here/'pioneer_q38_global_chain_role_test.py')],check=True)
with tempfile.TemporaryDirectory(prefix='pioneer-global-chain-data-',dir='/private/tmp') as tmp:
 for name,flags in (
  ('default',[]),
  ('scattered-cache',['-DPIONEER_Q38_GDN_SCATTERED_STATE_EXPERIMENT=1']),
  ('scattered-l1-authoritative',['-DPIONEER_Q38_GDN_SCATTERED_STATE_EXPERIMENT=1',
   '-DPIONEER_Q38_GDN_L1_NO_GDDR_WRITEBACK_EXPERIMENT=1'])):
  binary=pathlib.Path(tmp)/name
  subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',
   *flags,str(here/'pioneer_q38_global_chain_data_test.cpp'),'-o',str(binary)],check=True)
  subprocess.run([str(binary)],check=True,env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
