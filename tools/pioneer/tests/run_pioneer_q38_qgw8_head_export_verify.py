#!/usr/bin/env python3
"""Compile a private, CPU-only head payload verifier and run it once."""
import hashlib
import os
from pathlib import Path
import stat
import subprocess
import sys
import tempfile

if len(sys.argv) != 4 or any(not Path(x).is_absolute() for x in sys.argv[1:]):
    raise SystemExit('usage: ABS_PRIVATE_EXPORT_DIR ABS_GGUF ABS_PINNED_CPU_HIDDEN')
if any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ):
    raise SystemExit('Pioneer role override refused')
private, model, snapshot = map(Path, sys.argv[1:])
if private.is_symlink() or not private.is_dir() or stat.S_IMODE(private.stat().st_mode) != 0o700 or private.stat().st_uid != os.geteuid():
    raise SystemExit('private directory identity refused')
pins = {'PioneerNativeBfpModelBandData.h':'d3ff8eeb5830fb33f908a00d1d9dc17860724d41aedc2f0e0bf0045dd8deaae3',
        'PioneerNativeBfpModelBandMetadata.h':'664591584043b713148464b16054ba25d3456bcd603ad34686dc7208535d0fd9'}
def check_private():
    for name, digest in pins.items():
        path=private/name; info=path.lstat()
        if not stat.S_ISREG(info.st_mode) or info.st_nlink!=1 or info.st_uid!=os.geteuid() or stat.S_IMODE(info.st_mode)!=0o600:
            raise RuntimeError('unsafe private input: '+name)
        if hashlib.sha256(path.read_bytes()).hexdigest()!=digest:
            raise RuntimeError('private input hash mismatch: '+name)
check_private()
here=Path(__file__).resolve().parent
compiler=os.environ.get('CXX','c++')
flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-ffp-contract=off',
       '-fsanitize=address,undefined','-DPIONEER_QGW8_HEAD_EXPORT_VERIFY=1','-I'+str(private)]
source=str(here/'pioneer_q38_qgw8_head_export_verify.cpp')
for bad in ('_KERNEL_MODE','PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE',
            'PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_FULL_MODEL_CPU_REFERENCE','__FAST_MATH__'):
    result=subprocess.run([compiler,*flags,'-D'+bad+'=1','-fsyntax-only',source],capture_output=True,text=True)
    if result.returncode==0 or 'Separate offline head export verifier required' not in result.stderr:
        raise RuntimeError('role refusal not proved: '+bad)
with tempfile.TemporaryDirectory(prefix='qgw8-head-export-verify-',dir='/private/tmp') as tmp:
    exe=Path(tmp)/'verify'  # contains private model weights: never publish
    subprocess.run([compiler,*flags,source,str(here.parent/'PioneerModelWeightCache.cpp'),'-o',str(exe)],check=True)
    for args in ([], ['relative','relative'], ['/private/tmp/nonexistent-qgw8-model','/private/tmp/nonexistent-qgw8-snapshot']):
        if subprocess.run([str(exe),*args],capture_output=True).returncode==0:
            raise RuntimeError('invalid arguments accepted')
    subprocess.run([str(exe),str(model),str(snapshot)],check=True,
                   env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
check_private()
print('PASS private head exporter identity, independent tile verifier and role refusals; NO device')
