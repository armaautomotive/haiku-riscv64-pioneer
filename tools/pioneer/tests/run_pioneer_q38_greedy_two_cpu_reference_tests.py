#!/usr/bin/env python3
"""Build and test the isolated CPU reference; never open a real model or device."""
import os
from pathlib import Path
import subprocess
import tempfile

assert __debug__ and not os.environ.get('PYTHONOPTIMIZE')
assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
here=Path(__file__).resolve().parent
source=here/'pioneer_q38_greedy_two_cpu_reference.cpp'
compiler=os.environ.get('CXX','c++')
flags=['-std=c++17','-O2','-Wall','-Wextra','-Werror','-ffp-contract=off',
       '-DPIONEER_Q38_GREEDY_TWO_CPU_DIAGNOSTIC=1']
with tempfile.TemporaryDirectory(prefix='qgwd-greedy-two-',dir='/private/tmp') as directory:
    out=Path(directory)/'reference'
    subprocess.run([compiler,*flags,str(source),str(here/'pioneer_q38_cpu_model_reference.cpp'),
                    str(here/'pioneer_q38_whole_original_bytes.cpp'),str(here.parent/'PioneerModelWeightCache.cpp'),
                    '-o',str(out)],check=True)
    for args,expected in (([],2),(['relative.gguf'],2),([str(Path(directory)/'missing.gguf')],1)):
        result=subprocess.run([str(out),*args],capture_output=True)
        assert result.returncode==expected,(args,result.returncode)
    for forbidden in ('_KERNEL_MODE','PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE',
                      'PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_FULL_MODEL_CPU_REFERENCE','__FAST_MATH__'):
        result=subprocess.run([compiler,*flags,'-D'+forbidden+'=1','-fsyntax-only',str(source)],capture_output=True)
        assert result.returncode and b'isolated CPU-only diagnostic' in result.stderr,forbidden
    for bad in ([],['-DPIONEER_Q38_GREEDY_TWO_CPU_DIAGNOSTIC=0']):
        result=subprocess.run([compiler,'-std=c++17',*bad,'-fsyntax-only',str(source)],capture_output=True)
        assert result.returncode and b'isolated CPU-only diagnostic' in result.stderr
print('PASS isolated greedy-two CPU reference build, input refusals and role exclusions; NO MODEL OR DEVICE')
