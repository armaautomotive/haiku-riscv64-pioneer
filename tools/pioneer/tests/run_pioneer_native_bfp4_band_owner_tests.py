#!/usr/bin/env python3
"""Exercise fixed BFP4 synthetic owner with actual RV32 images; no device IO."""
import os
from pathlib import Path
import subprocess
import tempfile

here=Path(__file__).resolve().parent
sdk=Path('/Volumes/HaikuBuildLocal/generated.riscv64')
assert sdk.is_dir() and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
cxx=os.environ.get('CXX','c++')
with tempfile.TemporaryDirectory(prefix='pioneer-bfp4-owner-',dir='/private/tmp') as tmp:
    root=Path(tmp)
    for width in (5120,6144,17408):
        subprocess.run(['python3',str(here.parent/'build_native_bfp4_band.py'),str(sdk),str(root/str(width)),str(width)],check=True)
    test=root/'test'
    subprocess.run([cxx,'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-Wno-misleading-indentation','-Wno-unused-function',
                    '-ffp-contract=off','-fsanitize=address,undefined',str(here/'pioneer_native_bfp4_band_owner_test.cpp'),'-o',str(test)],check=True)
    subprocess.run([str(test),*(str(root/str(k)) for k in (5120,6144,17408))],check=True,
                   env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
    base=['-std=c++17','-fsyntax-only','-x','c++','-I'+str(here.parent),'-DB_DEVICE_OP_CODES_END=10000']
    new=['-D_KERNEL_MODE','-DPIONEER_NATIVE_BFP4_BAND_DIAGNOSTIC=1','-DTT_PIONEER_DRIVER_RELOAD',
         '-DPIONEER_DRIVER_BUILD_TAG=0x2026110600010118ULL']
    old=['-D_KERNEL_MODE','-DPIONEER_NATIVE_BFP_BAND_DIAGNOSTIC=1','-DTT_PIONEER_DRIVER_RELOAD',
         '-DPIONEER_DRIVER_BUILD_TAG=0x2026110400010079ULL']
    for flags,typename,success in ((new,'PioneerNativeBfp4BandLease',True),(new,'PioneerNativeBfpBandLease',False),
                                   (old,'PioneerNativeBfpBandLease',True),(old,'PioneerNativeBfp4BandLease',False),
                                   (new+['-DPIONEER_Q38_WORK_INFERENCE'],'PioneerNativeBfp4BandLease',False),
                                   (new+['-DPIONEER_Q38_CPU_VALIDATION'],'PioneerNativeBfp4BandLease',False),
                                   (['-D_KERNEL_MODE'],'PioneerNativeBfp4BandLease',False)):
        source='#include "PioneerNativeBfpBandLease.h"\nstatic_assert(sizeof('+typename+')>0,"instantiate");\n'
        p=subprocess.run([cxx,*base,*flags,'-'],input=source,text=True,capture_output=True)
        assert (p.returncode==0)==success,(flags,typename,p.stderr)
    print('PASS owner role isolation: exact 79/BFP8 and 118/BFP4 only; Work/validation/wrong format refused; NO device IO')
