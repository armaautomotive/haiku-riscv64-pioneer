#!/usr/bin/env python3
"""QGWE plan through synthetic QGWD upload/session; no device or real model."""
import hashlib
import os
from pathlib import Path
import subprocess
import sys
import tempfile

if len(sys.argv)!=2 or any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ):
    raise SystemExit('absolute pinned QGWE DATA header required')
header=Path(sys.argv[1])
if not header.is_absolute() or header.is_symlink() or hashlib.sha256(header.read_bytes()).hexdigest()!='5097fc1b4aa43bf1a9a1418345e138649293c8e2ee2cea7097b92989a7ba14bf':
    raise SystemExit('QGWE DATA identity refused')
here=Path(__file__).resolve().parent;source=here/'pioneer_q38_qgwd_full_session_test.cpp'
with tempfile.TemporaryDirectory(prefix='qgwe-full-session-',dir='/private/tmp') as tmp:
    binary=Path(tmp)/'test'
    flags=['-DPIONEER_Q38_QGWE_BATCH_DENSE_PLAN=1','-DPIONEER_Q38_QGWD_MULTI_TOKEN_SIMULATION=1']
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O2','-g','-Wall','-Wextra','-Werror',
                    '-Wno-misleading-indentation','-Wno-unused-function','-fsanitize=address,undefined',
                    *flags,str(source),str(here.parent/'PioneerModelWeightCache.cpp'),'-o',str(binary)],check=True)
    env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'}
    for mode in ('','--fail-start','--bad-feedback'):
        subprocess.run([str(binary),*([mode] if mode else [])],check=True,env=env)
print('PASS QGWE-plan synthetic upload/full-session lifecycle; DATA hash checked but NOT executed; NO DEVICE OR TPS')
