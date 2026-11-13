#!/usr/bin/env python3
"""Test the host-only BFP8 queued-read/tensor bridge."""
import os
from pathlib import Path
import subprocess
import tempfile

here=Path(__file__).resolve().parent;compiler=os.environ.get('CXX','c++')
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
with tempfile.TemporaryDirectory(prefix='q38-native-bfp-peer-',dir='/private/tmp') as tmp:
    binary=str(Path(tmp)/'test')
    subprocess.run([compiler,'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-Wno-misleading-indentation',
                    '-Wno-unused-function','-ffp-contract=off','-fsanitize=address,undefined',
                    '-DPIONEER_Q38_BFP8_BATCH_INPUT_EXPERIMENT=1',
                    str(here/'pioneer_q38_native_bfp_peer_bridge.cpp'),str(here/'pioneer_q38_native_bfp_peer_bridge_test.cpp'),'-o',binary],check=True)
    subprocess.run([binary],check=True,env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
    recurrent_binary=str(Path(tmp)/'recurrent-test')
    subprocess.run([compiler,'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-Wno-misleading-indentation',
                    '-Wno-unused-function','-ffp-contract=off','-fsanitize=address,undefined',
                    '-DPIONEER_Q38_BFP8_BATCH_INPUT_EXPERIMENT=1','-DPIONEER_Q38_RECURRENT_BFP8_WIDTH_INSPECTION=1',
                    str(here/'pioneer_q38_native_bfp_peer_bridge.cpp'),str(here/'pioneer_q38_native_bfp_peer_bridge_test.cpp'),'-o',recurrent_binary],check=True)
    subprocess.run([recurrent_binary],check=True,env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
    for forbidden in ('_KERNEL_MODE','PIONEER_DRIVER_BUILD_TAG','PIONEER_Q38_TOKEN_NATIVE_CONTEXT','PIONEER_Q38_WORK_INFERENCE',
                      'TT_PIONEER_Q38_WORK_MODE','PIONEER_Q38_FULL_OFFLOAD_NATIVE','PIONEER_Q38_FULL_OFFLOAD_CLIENT',
                      'PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_FULL_MODEL_CPU_REFERENCE'):
        result=subprocess.run([compiler,'-std=c++17','-DPIONEER_Q38_BFP8_BATCH_INPUT_EXPERIMENT=1','-D'+forbidden+'=1',
                               '-E',str(here/'pioneer_q38_native_bfp_peer_bridge.cpp')],capture_output=True,text=True)
        assert result.returncode and 'Native BFP peer backend is an isolated host instruction test' in result.stderr
print('PASS host BFP8 peer backend role isolation; NO DEVICE')
