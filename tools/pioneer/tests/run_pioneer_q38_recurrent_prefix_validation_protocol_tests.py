#!/usr/bin/env python3
"""Protocol DATA/client behavior tests over synthetic raw bytes and OS replies."""
import os
import pathlib
import subprocess
import tempfile
here=pathlib.Path(__file__).resolve().parent
with tempfile.TemporaryDirectory(prefix='pioneer-prefix-validation-',dir='/private/tmp') as tmp:
 for name in ('protocol','session','source'):
  binary=pathlib.Path(tmp)/name
  subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-ffp-contract=off',
   '-fsanitize=address,undefined',str(here/('pioneer_q38_recurrent_prefix_validation_'+name+'_test.cpp')),
   *([str(here.parent/'pioneer_q38_recurrent_prefix_validation_source.cpp')] if name=='source' else []),'-o',str(binary)],check=True)
  subprocess.run([str(binary)],check=True,env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
