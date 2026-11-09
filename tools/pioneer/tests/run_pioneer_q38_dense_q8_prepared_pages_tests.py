#!/usr/bin/env python3
"""Exercise dense Q8 prepared-page routing with fake cache/source only."""
import os
from pathlib import Path
import subprocess
import tempfile
here=Path(__file__).resolve().parent;root=here.parent
if any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ):raise SystemExit('role override refused')
with tempfile.TemporaryDirectory(prefix='q38-dense-q8-pages-',dir='/private/tmp') as directory:
    binary=Path(directory)/'test'
    command=[os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror',
             '-Wno-misleading-indentation','-Wno-unused-function','-Wno-unused-variable','-fsanitize=address,undefined',
             str(here/'pioneer_q38_dense_q8_prepared_pages_test.cpp'),str(root/'PioneerModelWeightCache.cpp'),'-o',str(binary)]
    subprocess.run(command,check=True)
    subprocess.run([str(binary)],check=True,env=dict(os.environ,UBSAN_OPTIONS='halt_on_error=1'))
print('PASS synthetic dense Q8 page routing; NO MODEL OR DEVICE')
