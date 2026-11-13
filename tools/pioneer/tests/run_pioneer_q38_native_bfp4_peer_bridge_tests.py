#!/usr/bin/env python3
"""Test isolated BFP4 queued-read and tensor endpoints; never a device."""
import os
from pathlib import Path
import subprocess
import tempfile

here=Path(__file__).resolve().parent
compiler=os.environ.get('CXX','c++')
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
with tempfile.TemporaryDirectory(prefix='q38-native-bfp4-peer-',dir='/private/tmp') as tmp:
    binary=str(Path(tmp)/'test')
    flags=['-DPIONEER_Q38_NATIVE_BFP4_PEER_TEST=1']
    subprocess.run([compiler,'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-Wno-misleading-indentation',
                    '-Wno-unused-function','-ffp-contract=off','-fsanitize=address,undefined',*flags,
                    str(here/'pioneer_q38_native_bfp4_peer_bridge.cpp'),str(here/'pioneer_q38_native_bfp4_peer_bridge_test.cpp'),'-o',binary],check=True)
    subprocess.run([binary],check=True,env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
    for forbidden in ('_KERNEL_MODE','PIONEER_DRIVER_BUILD_TAG','PIONEER_Q38_TOKEN_NATIVE_CONTEXT','PIONEER_Q38_WORK_INFERENCE',
                      'TT_PIONEER_Q38_WORK_MODE','PIONEER_Q38_FULL_OFFLOAD_NATIVE','PIONEER_Q38_FULL_OFFLOAD_CLIENT',
                      'PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_FULL_MODEL_CPU_REFERENCE'):
        result=subprocess.run([compiler,'-std=c++17',*flags,'-D'+forbidden+'=1','-E',
                               str(here/'pioneer_q38_native_bfp4_peer_bridge.cpp')],capture_output=True,text=True)
        assert result.returncode and 'Native BFP4 peer backend requires an isolated host test' in result.stderr
print('PASS host BFP4 peer backend role isolation; NO DEVICE')
