#!/usr/bin/env python3
"""Cache preparation stays outside kernels,shader Work and CPU validation."""
import os
import pathlib
import subprocess
import sys
import tempfile

if not __debug__ or os.environ.get('PYTHONOPTIMIZE'):
    raise SystemExit('Build-gate assertions required')
here=pathlib.Path(__file__).resolve().parent;root=here.parent
compiler=os.environ.get('CXX','c++')
with tempfile.TemporaryDirectory(prefix='pioneer-native-q4-cache-gates-',dir='/private/tmp') as temporary:
    directory=pathlib.Path(temporary);cases=0
    for flags,expected in (
        (['-D_KERNEL_MODE=1'],'Disk cache is CPU/userspace only'),
        (['-DPIONEER_ATTENTION_FREESTANDING=1'],'Disk cache is CPU/userspace only'),
        (['-DPIONEER_Q38_WORK_INFERENCE=1'],'Exact Q4 source-format prerequisite only'),
        (['-DPIONEER_Q38_CPU_VALIDATION=1'],'Exact Q4 source-format prerequisite only'),
        (['-ffast-math'],'Exact Q4 source-format prerequisite only')):
        result=subprocess.run([compiler,'-std=c++17','-fsyntax-only','-x','c++',*flags,
                               str(root/'PioneerQ38NativeQ4CachedPages.h')],capture_output=True,text=True)
        assert result.returncode and expected in result.stderr
        cases+=1
    result=subprocess.run([compiler,'-std=c++17','-DNDEBUG','-fsyntax-only','-x','c++',
                           str(here/'pioneer_q38_native_q4_weight_cache_test.cpp')],capture_output=True,text=True)
    assert result.returncode and 'Cache validation checks must remain enabled' in result.stderr;cases+=1
    result=subprocess.run([sys.executable,'-O',str(root/'build_native_q4_preparation.py'),str(directory/'forbidden-build')],capture_output=True,text=True)
    assert result.returncode and 'Refuse optimized Python' in result.stderr and not (directory/'forbidden-build').exists();cases+=1
print('PASS_NATIVE_Q4_CACHE_BUILD_GATES refusals'+str(cases)+';kernel_shaderWork_CPUvalidation_fastmath_disabledChecks_refused;NO_NATIVE_OR_TPS',flush=True)
