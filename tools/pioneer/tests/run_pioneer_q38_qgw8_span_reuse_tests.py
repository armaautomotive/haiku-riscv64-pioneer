#!/usr/bin/env python3
"""Simulate the existing QGW8 span service with optional code-proof reuse."""
import hashlib
import os
from pathlib import Path
import subprocess
import sys
import tempfile

if len(sys.argv)!=2 or not Path(sys.argv[1]).is_absolute() or not Path(sys.argv[1]).is_dir():
    raise SystemExit('absolute Haiku SDK directory required')
if any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ):
    raise SystemExit('role override refused')
here=Path(__file__).resolve().parent;root=here.parent
with tempfile.TemporaryDirectory(prefix='qgw8-span-reuse-',dir='/private/tmp') as directory:
    tmp=Path(directory);image=tmp/'image';binary=tmp/'test'
    subprocess.run(['sh',str(root/'build_qwen38_span_image.sh'),sys.argv[1],str(image)],check=True,
                   env=dict(os.environ,PIONEER_Q38_SPAN_FAST='1'))
    for name,digest in (('boot.bin','e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b'),
                        ('text.bin','9b4e950f3d016d229b7f9ed7c5dd9f2e0fc23a2865a5f1a36f6633216f397609')):
        assert hashlib.sha256((image/name).read_bytes()).hexdigest()==digest
    flags=['-std=c++17','-O1','-g','-ffp-contract=off','-Wall','-Wextra','-Werror',
           '-Wno-unused-function','-Wno-unused-variable','-Wno-misleading-indentation',
           '-DTT_PIONEER_EXPERIMENTAL_Q38_BANKS','-DPIONEER_Q38_SPAN_FAST=1',
           '-DPIONEER_Q38_SPAN_REUSE_CODE=1','-DPIONEER_TEST_QGW8_FAST=1','-DPIONEER_TEST_QGW8_REUSE=1']
    source=str(here/'pioneer_q38_qgw8_tracked_span_test.cpp');compiler=os.environ.get('CXX','c++')
    subprocess.run([compiler,*flags,'-fsanitize=address,undefined',source,'-o',str(binary)],check=True)
    subprocess.run([str(binary),str(image)],check=True,
                   env=dict(os.environ,ASAN_OPTIONS='quarantine_size_mb=32',UBSAN_OPTIONS='halt_on_error=1'))
    for required in ('PIONEER_Q38_SPAN_FAST','PIONEER_Q38_SPAN_REUSE_CODE','PIONEER_TEST_QGW8_FAST'):
        reduced=[f for f in flags if f!='-D'+required+'=1']
        result=subprocess.run([compiler,*reduced,'-fsyntax-only',source],capture_output=True,text=True)
        assert result.returncode and 'QGW8 reuse test requires' in result.stderr,required
print('PASS QGW8 loader reuse synthetic proof and role refusals; NO DEVICE OR SPEED RESULT')
