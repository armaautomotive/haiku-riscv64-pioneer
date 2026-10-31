#!/usr/bin/env python3
"""Synthetic host observation arithmetic only, not accelerator measurements."""
import os
import pathlib
import subprocess
import tempfile
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
here=pathlib.Path(__file__).resolve().parent
with tempfile.TemporaryDirectory(prefix='pioneer-q38-token-rate-',dir='/private/tmp') as name:
    binary=pathlib.Path(name)/'test'
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror',
                    '-fsanitize=address,undefined',str(here/'pioneer_q38_token_rate_samples_test.cpp'),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True,env={**os.environ,'UBSAN_OPTIONS':'halt_on_error=1'})
    include='#include "'+str(here.parent/'PioneerQ38TokenRateSamples.h')+'"\n'
    for value in ('0','1'):
        for late in (False,True):
            source=(include if late else '')+'#define _KERNEL_MODE '+value+'\n'+include
            result=subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-fsyntax-only','-x','c++','-'],
                                  input=source,text=True,capture_output=True)
            assert result.returncode and 'Host observation arithmetic is NOT kernel code' in result.stderr
print('PASS host-only sampler/kernel inclusion refused;NO device/model/source/native cleanup or actual timing authority')
