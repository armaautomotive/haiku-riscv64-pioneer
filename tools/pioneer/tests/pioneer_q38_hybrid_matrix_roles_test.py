#!/usr/bin/env python3
"""Fail closed on QGW7/QDM2 identity and old/native/Work role mixtures."""
from pathlib import Path
import os
import subprocess
import sys

if len(sys.argv) != 2:
    raise SystemExit('usage: ... ABSOLUTE_SDK')
sdk=Path(sys.argv[1]);assert sdk.is_absolute() and sdk.is_dir()
root=Path(__file__).resolve().parent.parent
compiler=str(sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku-g++')
global_source=root/'blackhole_qwen38_global_worker.cpp'
dense_source=root/'blackhole_qwen38_dense_partition.cpp'
global_base=['-DPIONEER_Q38_BF16_WORKER_IMAGE=1','-DPIONEER_Q38_HYBRID_WORKER_IMAGE=1',
             '-DPIONEER_DRAM_SWEEP_FREESTANDING=1']
dense_base=['-DPIONEER_Q38_MIXED_WORKER_IMAGE=1','-DPIONEER_Q38_MIXED_DENSE_CAPSULE_IMAGE=1',
            '-DPIONEER_Q38_MIXED_CAPSULE_IMAGE=1']
matrix='-DPIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT=1'
identity='-DPIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION=1'
def preprocess(source,flags,success):
    result=subprocess.run([compiler,'-std=c++11','-E',*flags,str(source)],stdout=subprocess.DEVNULL,stderr=subprocess.PIPE)
    assert (result.returncode==0)==success,(source,flags,result.stderr[-1200:])
    if not success: assert b'error:' in result.stderr
preprocess(global_source,[*global_base,matrix,identity],True)
preprocess(dense_source,[*dense_base,matrix,identity],True)
negative=[
 (global_source,[*global_base,matrix]),
 (global_source,[*global_base,identity]),
 (global_source,[*global_base,matrix,'-DPIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION=0']),
 (global_source,[*global_base,matrix,identity,'-DPIONEER_Q38_HYBRID_INPUT_REUSE_INSPECTION=1']),
 (dense_source,[*dense_base,matrix]),
 (dense_source,[*dense_base,identity]),
 (dense_source,['-DPIONEER_Q38_DENSE_PARTITION_IMAGE=1',matrix,identity]),
]
for source,base in ((global_source,global_base),(dense_source,dense_base)):
    for macro in ('_KERNEL_MODE','PIONEER_DRIVER_BUILD_TAG','PIONEER_Q38_WORK_INFERENCE',
                  'TT_PIONEER_Q38_WORK_MODE','PIONEER_Q38_CPU_VALIDATION',
                  'PIONEER_Q38_FULL_MODEL_CPU_REFERENCE'):
        for value in (0,1): negative.append((source,[*base,matrix,identity,'-D'+macro+'='+str(value)]))
for source,flags in negative:
    preprocess(source,flags,False)
# Exercise the shared coordinator grammar itself, not just the worker source
# image gates. A QDM2 inner frame must never inherit the old QGW5 identity.
grammar_cases=[([],True),([matrix,identity],True),([matrix],False),([identity],False),
               ([matrix,identity,'-DPIONEER_Q38_HYBRID_INPUT_REUSE_INSPECTION=1'],False)]
for macro in ('_KERNEL_MODE','PIONEER_DRIVER_BUILD_TAG','PIONEER_Q38_HYBRID_KERNEL_COMPILE_ONLY',
              'PIONEER_Q38_HYBRID_NATIVE_COMPILE_ONLY','PIONEER_Q38_HYBRID_FULL_NATIVE'):
    for value in (0,1):grammar_cases.append(([matrix,identity,'-D'+macro+'='+str(value)],False))
for flags,success in grammar_cases:
    result=subprocess.run([os.environ.get('CXX','c++'),'-std=c++11','-E','-x','c++','-I',str(root),*flags,'-'],
                          input=b'#include "PioneerQ38HybridWorkerLayout.h"\n',
                          stdout=subprocess.DEVNULL,stderr=subprocess.PIPE)
    assert (result.returncode==0)==success,(flags,result.stderr[-1200:])
print('PASS QGW7/QDM2 matching role, '+str(len(negative))+' incompatible worker builds and '+str(len(grammar_cases))+' direct coordinator-grammar gates; NO DEVICE')
