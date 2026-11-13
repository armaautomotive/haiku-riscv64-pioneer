#!/usr/bin/env python3
"""Test recurrent prepared pages through synthetic spans, never a device."""
import os
from pathlib import Path
import subprocess
import tempfile

here = Path(__file__).resolve().parent
source = here / 'pioneer_q38_recurrent_q8_prepared_upload_test.cpp'
protocol = here.parent / 'PioneerQ38RecurrentQ8UploadProtocolInspection.h'
assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
compiler = os.environ.get('CXX', 'c++')
with tempfile.TemporaryDirectory(prefix='q38-recurrent-upload-', dir='/private/tmp') as directory:
    binary = str(Path(directory) / 'test')
    subprocess.run([compiler, '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                    '-Wno-misleading-indentation', '-Wno-unused-function', '-Wno-unused-variable',
                    '-fsanitize=address,undefined', str(source), str(here.parent/'PioneerModelWeightCache.cpp'),
                    '-o', binary], check=True)
    subprocess.run([binary], check=True, env=dict(os.environ, UBSAN_OPTIONS='halt_on_error=1'))
    for forbidden in ('_KERNEL_MODE', 'PIONEER_DRIVER_BUILD_TAG', 'PIONEER_Q38_TOKEN_NATIVE_CONTEXT',
                      'PIONEER_Q38_WORK_INFERENCE', 'TT_PIONEER_Q38_WORK_MODE',
                      'PIONEER_Q38_CPU_VALIDATION', 'PIONEER_Q38_FULL_MODEL_CPU_REFERENCE'):
        result = subprocess.run([compiler, '-std=c++17', '-DPIONEER_Q38_HYBRID_UPLOAD_DIAGNOSTIC=1',
                                 '-D'+forbidden+'=1', '-E', '-x', 'c++', '-'],
                                input='#include "'+str(here.parent/'PioneerQ38HybridSpanUpload.h')+'"\n',
                                text=True, capture_output=True)
        assert result.returncode and 'Hybrid span upload is a separate experimental diagnostic' in result.stderr, forbidden
        isolated = subprocess.run([compiler, '-std=c++17', '-DPIONEER_Q38_RECURRENT_Q8_UPLOAD_INSPECTION=1',
                                   '-D'+forbidden+'=1', '-E', '-x', 'c++', '-'],
                                  input='#include "'+str(protocol)+'"\n', text=True, capture_output=True)
        assert isolated.returncode and 'Recurrent upload protocol is offline inspection only' in isolated.stderr, forbidden
    late = subprocess.run([compiler, '-std=c++17', '-fsyntax-only', '-x', 'c++', '-'],
                          input='#define PIONEER_Q38_RECURRENT_Q8_UPLOAD_INSPECTION 1\n#include "'+str(protocol)+'"\n'
                                '#undef PIONEER_Q38_RECURRENT_Q8_UPLOAD_INSPECTION\n#include "'+str(protocol)+'"\n',
                          text=True, capture_output=True)
    assert late.returncode and 'Recurrent upload protocol is offline inspection only' in late.stderr
print('PASS recurrent upload excludes native, Work and reference roles; NO DEVICE')
