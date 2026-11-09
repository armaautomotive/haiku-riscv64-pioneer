#!/usr/bin/env python3
"""Exercise QGWA-plan metadata through the existing synthetic token session."""
import hashlib
import os
from pathlib import Path
import subprocess
import sys
import tempfile

if len(sys.argv)!=2 or any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ):
    raise SystemExit('absolute pinned QGWA data header required')
header=Path(sys.argv[1]);here=Path(__file__).resolve().parent;root=here.parent
if not header.is_absolute() or header.is_symlink() or hashlib.sha256(header.read_bytes()).hexdigest()!='b615fda5f32852814f5383c6ec66c7a80a4c344cd4e7e196a593825beac462d8':
    raise SystemExit('QGWA data identity refused')
# The session is fake-tag inspection only. Program DATA is checked above;
# the actual startup/119-boot binding is separately covered by the held
# head ROOT bridge. No simulated reply is evidence of physical execution.
flags=['-DPIONEER_Q38_QGWA_BATCH_ROOT_RECEIPT_INSPECTION=1']
with tempfile.TemporaryDirectory(prefix='qgwa-full-session-',dir='/private/tmp') as directory:
    binary=Path(directory)/'test'
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O2','-g','-Wall','-Wextra','-Werror',
                    '-Wno-misleading-indentation','-Wno-unused-function','-Wno-unused-variable','-ffp-contract=off',
                    '-fsanitize=address,undefined',*flags,str(here/'pioneer_q38_qgw8_full_session_test.cpp'),
                    str(root/'PioneerModelWeightCache.cpp'),'-o',str(binary)],check=True)
    env=dict(os.environ,ASAN_OPTIONS='quarantine_size_mb=32',UBSAN_OPTIONS='halt_on_error=1')
    for mode in ('--wrong-tag','--bad-support','','--bad-clock','--fail-start','--bad-record'):
        subprocess.run([str(binary),*([mode] if mode else [])],check=True,env=env)
print('PASS QGWA-plan synthetic upload/full-session lifecycle and fault refusals; NO DEVICE OR TPS')
