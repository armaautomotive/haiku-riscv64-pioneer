#!/usr/bin/env python3
"""Compare controller and host dense Q8 job geometry; no device."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile
if any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ):raise SystemExit('role override refused')
if len(sys.argv)>2:raise SystemExit('usage: test [absolute SDK path]')
here=Path(__file__).resolve().parent
with tempfile.TemporaryDirectory(prefix='q38-dense-q8-root-jobs-',dir='/private/tmp') as directory:
    binary=Path(directory)/'test'
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror',
                    '-Wno-misleading-indentation','-Wno-unused-function','-Wno-unused-variable',
                    '-fsanitize=address,undefined',str(here/'pioneer_q38_dense_q8_matrix_root_jobs_test.cpp'),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True,env=dict(os.environ,UBSAN_OPTIONS='halt_on_error=1'))
    header=here.parent/'PioneerQ38DenseQ8MatrixRootJobs.h'
    compiler=os.environ.get('CXX','c++');base=['-DPIONEER_Q38_DENSE_Q8_MATRIX_ROOT_INSPECTION=1']
    subprocess.run([compiler,'-std=c++17',*base,'-fsyntax-only','-x','c++',str(header)],check=True)
    for role in ('_KERNEL_MODE','PIONEER_DRIVER_BUILD_TAG','PIONEER_Q38_TOKEN_NATIVE_CONTEXT',
                 'PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE','PIONEER_Q38_CPU_VALIDATION',
                 'PIONEER_Q38_FULL_MODEL_CPU_REFERENCE'):
        result=subprocess.run([compiler,'-std=c++17',*base,'-D'+role+'=1','-fsyntax-only','-x','c++',str(header)],capture_output=True)
        assert result.returncode and b'Dense Q8 ROOT scheduler is offline inspection only' in result.stderr,role
    if len(sys.argv)==2:
        sdk=Path(sys.argv[1]);assert sdk.is_absolute() and sdk.is_dir()
        tool=sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku';obj=Path(directory)/'frame.o'
        subprocess.run([str(tool)+'-g++','-std=c++11','-O2','-ffreestanding','-fno-exceptions','-fno-rtti',
                        '-fno-stack-protector','-fno-pic','-fno-pie','-fno-builtin','-ffunction-sections','-fdata-sections',
                        '-mno-relax','-march=rv32imf','-mabi=ilp32','-fstack-usage','-c',
                        str(here/'pioneer_q38_dense_q8_root_frame_probe.cpp'),'-o',str(obj)],check=True)
        undefined=subprocess.check_output([str(tool)+'-nm','-u',str(obj)],text=True).splitlines()
        assert [line.split()[-1] for line in undefined]==['memset'],undefined
        frames=(Path(directory)/'frame.su').read_text().splitlines()
        assert frames and all(line.split('\t')[-1]=='static' and int(line.split('\t')[-2])<=2048 for line in frames)
        print('PASS isolated RV32 frame probe build; memset must be supplied by ROOT; NOT LINKED OR EXECUTED')
print('PASS synthetic dense Q8 controller/manifest bridge; NO DEVICE')
