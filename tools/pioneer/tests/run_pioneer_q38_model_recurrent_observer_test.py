#!/usr/bin/env python3
"""Validate host-only observer against pinned model-derived export references."""
import hashlib
import os
from pathlib import Path
import subprocess
import sys
import tempfile

assert len(sys.argv)==3 and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
paths=[Path(p) for p in sys.argv[1:]]
pins=('92f7287e9626c4dae6ff4c1118d6d2c64f194cf1c239ac3f7b1b60e477ed4fbb',
      'f6615b16825dbe365aad7d01729b2b6cbd19c2f0d51545890d1fe74933bcb753')
def validate():
    for p,digest in zip(paths,pins):
        assert p.is_absolute() and p.is_file() and not p.is_symlink() and hashlib.sha256(p.read_bytes()).hexdigest()==digest
validate()
here=Path(__file__).resolve().parent;source=here/'pioneer_q38_model_recurrent_observer.cpp'
compiler=os.environ.get('CXX','c++');flags=['-std=c++17','-O1','-Wall','-Wextra','-Werror','-Wno-misleading-indentation']
for role in ([],['-DPIONEER_Q38_MODEL_RECURRENT_OBSERVER_TEST=0'],
             *(['-DPIONEER_Q38_MODEL_RECURRENT_OBSERVER_TEST=1','-D'+name+'=1'] for name in ('_KERNEL_MODE','PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE'))):
    r=subprocess.run([compiler,*flags,*role,'-fsyntax-only',str(source)],capture_output=True,text=True)
    assert r.returncode and 'isolated host test' in r.stderr
with tempfile.TemporaryDirectory(prefix='q38-model-observer-',dir='/private/tmp') as tmp:
    binary=Path(tmp)/'test'
    subprocess.run([compiler,*flags,'-fsanitize=address,undefined','-DPIONEER_Q38_MODEL_RECURRENT_OBSERVER_TEST=1',
                    str(source),str(here/'pioneer_q38_model_recurrent_observer_test.cpp'),'-o',str(binary)],check=True)
    subprocess.run([str(binary),*map(str,paths)],check=True)
validate()
