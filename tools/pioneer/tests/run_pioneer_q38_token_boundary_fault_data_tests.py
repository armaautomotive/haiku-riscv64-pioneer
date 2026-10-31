#!/usr/bin/env python3
"""Actual USER failure-data helper under ASan/UBSan;NO native/device IO."""
import os
import pathlib
import subprocess
import tempfile
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
here=pathlib.Path(__file__).resolve().parent
with tempfile.TemporaryDirectory(prefix='pioneer-boundary-fault-data-',dir='/private/tmp') as name:
    binary=pathlib.Path(name)/'test'
    common=[os.environ.get('CXX','c++'),'-std=c++17','-Wall','-Wextra','-Werror']
    subprocess.run([*common,'-O1','-g','-fsanitize=address,undefined',str(here/'pioneer_q38_token_boundary_fault_data_test.cpp'),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True,env={**os.environ,'UBSAN_OPTIONS':'halt_on_error=1'})
    include='#define B_DEVICE_OP_CODES_END 1000\n#include "'+str(here.parent/'PioneerQ38TokenBoundaryFaultData.h')+'"\n'
    base='#define PIONEER_Q38_TOKEN_BOUNDARY_VALIDATION_CLIENT 1\n'
    cases=[include,base.replace('CLIENT 1','CLIENT 0')+include]
    for role in ('_KERNEL_MODE','PIONEER_Q38_TOKEN_NATIVE_CONTEXT','PIONEER_Q38_TOKEN_FAULT_VALIDATION_NATIVE',
                 'PIONEER_Q38_TOKEN_FAULT_PROGRAMS','PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE'):
        cases += ['#define '+role+' '+value+'\n'+base+include for value in ('0','1')]
    for source in cases:
        r=subprocess.run([*common,'-fsyntax-only','-x','c++','-'],input=source,text=True,capture_output=True)
        assert r.returncode and 'error:' in r.stderr,r.stderr
    print('PASS USER-only failure-DATA role/14 missing+zero+kernel+native+Work refusals;NO new operation,retry or cleanup')
