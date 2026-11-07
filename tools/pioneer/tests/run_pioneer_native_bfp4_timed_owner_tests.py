#!/usr/bin/env python3
"""Execute timed BFP4 images through a simulated fail-closed owner."""
import os
from pathlib import Path
import subprocess
import tempfile

here=Path(__file__).resolve().parent
sdk=Path('/Volumes/HaikuBuildLocal/generated.riscv64')
assert sdk.is_dir() and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
cxx=os.environ.get('CXX','c++')
with tempfile.TemporaryDirectory(prefix='pioneer-bfp4-timed-owner-',dir='/private/tmp') as tmp:
    root=Path(tmp)
    for width in (5120,6144,17408):
        subprocess.run(['python3',str(here.parent/'build_native_bfp4_band.py'),str(sdk),str(root/str(width)),str(width),'--timed'],check=True)
    binary=root/'test'
    subprocess.run([cxx,'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-Wno-misleading-indentation','-Wno-unused-function',
                    '-ffp-contract=off','-fsanitize=address,undefined',str(here/'pioneer_native_bfp4_timed_owner_test.cpp'),'-o',str(binary)],check=True)
    subprocess.run([str(binary),*(str(root/str(k)) for k in (5120,6144,17408))],check=True,
                   env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
    base=['-std=c++17','-fsyntax-only','-x','c++','-I'+str(here.parent),'-DB_DEVICE_OP_CODES_END=10000']
    timed=['-D_KERNEL_MODE','-DPIONEER_NATIVE_BFP4_TIMED_DIAGNOSTIC=1','-DTT_PIONEER_DRIVER_RELOAD',
           '-DPIONEER_DRIVER_BUILD_TAG=0x2026110600010119ULL']
    untimed=['-D_KERNEL_MODE','-DPIONEER_NATIVE_BFP4_BAND_DIAGNOSTIC=1','-DTT_PIONEER_DRIVER_RELOAD',
             '-DPIONEER_DRIVER_BUILD_TAG=0x2026110600010118ULL']
    for flags,typename,success in ((timed,'PioneerNativeBfp4TimedLease',True),(timed,'PioneerNativeBfp4BandLease',False),
                                   (untimed,'PioneerNativeBfp4TimedLease',False),(untimed,'PioneerNativeBfp4BandLease',True),
                                   (timed+['-DPIONEER_Q38_WORK_INFERENCE'],'PioneerNativeBfp4TimedLease',False),
                                   (timed+['-DPIONEER_Q38_CPU_VALIDATION'],'PioneerNativeBfp4TimedLease',False),
                                   (['-D_KERNEL_MODE'],'PioneerNativeBfp4TimedLease',False)):
        source='#include "PioneerNativeBfpBandLease.h"\nstatic_assert(sizeof('+typename+')>0,"instantiate");\n'
        p=subprocess.run([cxx,*base,*flags,'-'],input=source,text=True,capture_output=True)
        assert (p.returncode==0)==success,(flags,typename,p.stderr)
    print('PASS timed/untimed role isolation and Work/validation refusals; NO device IO')
