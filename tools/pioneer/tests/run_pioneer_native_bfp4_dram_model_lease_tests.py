#!/usr/bin/env python3
"""PRIVATE real BFP4 samples + actual RV32 owner; fake MMIO only."""
import hashlib
import os
from pathlib import Path
import stat
import subprocess
import sys
import tempfile

assert __debug__ and len(sys.argv)==3 and not os.environ.get('PYTHONOPTIMIZE')
sdk,private=map(Path,sys.argv[1:])
assert sdk.is_absolute() and sdk.is_dir() and private.is_absolute() and not private.is_symlink()
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
info=private.stat()
assert stat.S_ISDIR(info.st_mode) and info.st_uid==os.geteuid() and stat.S_IMODE(info.st_mode)==0o700
pins={'PioneerNativeBfp4DramModelData.h':'51bd25cb2a624a4f979eb93b1b2eca2dcd72b5076bb354ad6480595ac9b74298',
      'PioneerNativeBfp4DramModelMetadata.h':'9842393b3be3329d272cd9d9d358d3194b4d5c31aeb39f43728241a323a8ffa2'}
def unchanged():
    for name,digest in pins.items():
        path=private/name
        info=path.lstat()
        assert stat.S_ISREG(info.st_mode) and info.st_uid==os.geteuid() and info.st_nlink==1 and stat.S_IMODE(info.st_mode)==0o600
        assert hashlib.sha256(path.read_bytes()).hexdigest()==digest
unchanged()
here=Path(__file__).resolve().parent
with tempfile.TemporaryDirectory(prefix='pioneer-bfp4-dram-lease-',dir='/private/tmp') as temp:
    root=Path(temp)
    subprocess.run([sys.executable,str(here.parent/'build_bfp4_dram_band.py'),str(sdk),str(root/'image'),'5120'],check=True)
    assert hashlib.sha256((root/'image/text.bin').read_bytes()).hexdigest()=='78bbf79aa2fa3353dc4f8958acda3b5613449fbd07b313e56d463d1d02792ade'
    compiler=os.environ.get('CXX','c++')
    common=[compiler,'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-Wno-misleading-indentation','-Wno-unused-function',
            '-ffp-contract=off','-fsanitize=address,undefined','-I'+str(private),'-I'+str(here.parent)]
    subprocess.run([*common,str(here/'pioneer_native_bfp4_dram_model_lease_test.cpp'),'-o',str(root/'test')],check=True)
    subprocess.run([str(root/'test'),str(root/'image')],check=True,
                   env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
    source='#define B_DEVICE_OP_CODES_END 10000\n#include "PioneerNativeBfp4DramModelLease.h"\n'
    for macro in ('_KERNEL_MODE','PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE','PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_FULL_MODEL_CPU_REFERENCE'):
        for value in ('0','1'):
            result=subprocess.run([*common,'-D'+macro+'='+value,'-fsyntax-only','-x','c++','-'],input=source,text=True,capture_output=True)
            assert result.returncode and 'BFP4 GDDR lease requires its separate diagnostic' in result.stderr,(macro,value,result.stderr)
    role=['-D_KERNEL_MODE','-DPIONEER_NATIVE_BFP4_DRAM_DIAGNOSTIC=1','-DTT_PIONEER_DRIVER_RELOAD',
          '-DPIONEER_DRIVER_BUILD_TAG=0x2026110600010120ULL']
    result=subprocess.run([*common,*role,'-fsyntax-only','-x','c++','-'],input=source,text=True,capture_output=True)
    assert result.returncode==0,result.stderr
    for extra in (['-UPIONEER_NATIVE_BFP4_DRAM_DIAGNOSTIC'],['-UPIONEER_NATIVE_BFP4_DRAM_DIAGNOSTIC','-DPIONEER_NATIVE_BFP4_DRAM_DIAGNOSTIC=0'],
                  ['-UTT_PIONEER_DRIVER_RELOAD'],['-UPIONEER_DRIVER_BUILD_TAG','-DPIONEER_DRIVER_BUILD_TAG=0x2026110400010084ULL']):
        result=subprocess.run([*common,*role,*extra,'-fsyntax-only','-x','c++','-'],input=source,text=True,capture_output=True)
        assert result.returncode,(extra,result.stderr)
unchanged()
print('PASS BFP4 GDDR owner role binding and private-header identity; OFFBOARD only, no card/driver/Work authorization')
