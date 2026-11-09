#!/usr/bin/env python3
"""Test head/legacy C64 leases with private head bands; simulated I/O only."""
import hashlib
import os
from pathlib import Path
import stat
import subprocess
import sys
import tempfile

if len(sys.argv)!=3 or any(not Path(x).is_absolute() for x in sys.argv[1:]):
    raise SystemExit('usage: ABS_PRIVATE_HEAD_EXPORT ABS_PINNED_C64_IMAGE')
if any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ):
    raise SystemExit('Pioneer environment override refused')
private,image=map(Path,sys.argv[1:])
if private.is_symlink() or not private.is_dir() or private.stat().st_uid!=os.geteuid() or stat.S_IMODE(private.stat().st_mode)!=0o700:
    raise SystemExit('private directory refused')
pins={'PioneerNativeBfpModelBandData.h':'d3ff8eeb5830fb33f908a00d1d9dc17860724d41aedc2f0e0bf0045dd8deaae3',
      'PioneerNativeBfpModelBandMetadata.h':'664591584043b713148464b16054ba25d3456bcd603ad34686dc7208535d0fd9'}
def check():
    for name,pin in pins.items():
        path=private/name; info=path.lstat()
        if not stat.S_ISREG(info.st_mode) or info.st_uid!=os.geteuid() or info.st_nlink!=1 or stat.S_IMODE(info.st_mode)!=0o600:
            raise RuntimeError('private input refused: '+name)
        if hashlib.sha256(path.read_bytes()).hexdigest()!=pin: raise RuntimeError('private hash mismatch: '+name)
    for name,pin in (('boot.bin','d47a1c29c265663f5bd4bd49aaf28248b7345d7feb5a4f770caa27315deaa4dd'),
                     ('text.bin','7b3ca669be8d9e0714f544bd0d74c045329b0490d8f2132187181a1a92be55cd')):
        if hashlib.sha256((image/name).read_bytes()).hexdigest()!=pin: raise RuntimeError('image hash mismatch: '+name)
check()
here=Path(__file__).resolve().parent
subprocess.run([sys.executable,str(here/'pioneer_qgw8_head_band_role_test.py')],check=True)
env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'}
flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-ffp-contract=off',
       '-fsanitize=address,undefined','-DPIONEER_NATIVE_DRAM_MODEL_BAND_ALIGNMENT64=1',
       '-I'+str(private),'-I'+str(here.parent)]
compiler=os.environ.get('CXX','c++')
for role,change in (([], '#define PIONEER_QGW8_HEAD_BAND_130 1'),
                    (['-DPIONEER_QGW8_HEAD_BAND_130=1'], '#undef PIONEER_QGW8_HEAD_BAND_130')):
    source=('#define B_DEVICE_OP_CODES_END 10000\n#include "PioneerNativeDramModelBandLease.h"\n'
            +change+'\n#include "PioneerNativeDramModelBandLease.h"\n')
    result=subprocess.run([compiler,*flags,*role,'-x','c++','-fsyntax-only','-'],input=source,text=True,capture_output=True)
    if result.returncode==0 or 'late ' not in result.stderr:
        raise RuntimeError('late lease selector refusal not proved: '+result.stderr[:1000])
with tempfile.TemporaryDirectory(prefix='qgw8-head-lease-',dir='/private/tmp') as tmp:
    for name,role in (('legacy84',[]),('head130',['-DPIONEER_QGW8_HEAD_BAND_130=1'])):
        exe=Path(tmp)/name  # contains model weights; never publish
        subprocess.run([compiler,*flags,*role,
                        str(here/'pioneer_native_dram_model_band_lease_test.cpp'),'-o',str(exe)],check=True)
        subprocess.run([str(exe),str(image)],env=env,check=True)
check()
print('PASS head130 and legacy84 C64 lease with real head fixture; simulated only, no device')
