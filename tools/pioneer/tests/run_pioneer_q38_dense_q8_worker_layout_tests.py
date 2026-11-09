#!/usr/bin/env python3
"""Verify isolated dense Q8 worker grammar against all planned jobs."""
import os
from pathlib import Path
import subprocess
import tempfile
if any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ):raise SystemExit('role override refused')
here=Path(__file__).resolve().parent
with tempfile.TemporaryDirectory(prefix='q38-q8-worker-grammar-',dir='/private/tmp') as directory:
    binary=Path(directory)/'test'
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror',
                    '-Wno-misleading-indentation','-Wno-unused-function','-Wno-unused-variable',
                    '-fsanitize=address,undefined',str(here/'pioneer_q38_dense_q8_worker_layout_test.cpp'),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True,env=dict(os.environ,UBSAN_OPTIONS='halt_on_error=1'))
    prefix='''#define PIONEER_Q38_DENSE_Q8_MATRIX_WORKER_INSPECTION 1
#define PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION 1
#define PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION 1
#define PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT 1
#include "PioneerQ38DenseQ8MatrixWorkerLayout.h"
'''
    for late in ('#undef PIONEER_Q38_DENSE_Q8_MATRIX_WORKER_INSPECTION',
                 '#define PIONEER_Q38_WORK_INFERENCE 1',
                 '#define PIONEER_Q38_QGW8_BFP8_BATCH_WORKER 1'):
        source=prefix+late+'\n#include "PioneerQ38DenseQ8MatrixWorkerLayout.h"\n'
        result=subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-fsyntax-only','-x','c++',
                               '-I',str(here.parent),'-'],input=source,text=True,capture_output=True)
        assert result.returncode!=0 and 'isolated offline inspection only' in result.stderr,late
    source=prefix+'#undef PIONEER_Q38_DENSE_Q8_MATRIX_WORKER_INSPECTION\n#include "PioneerQ38Bf16ProducerLayout.h"\n'
    result=subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-fsyntax-only','-x','c++',
                           '-I',str(here.parent),'-'],input=source,text=True,capture_output=True)
    assert result.returncode and 'late dense Q8 producer removal' in result.stderr
print('PASS synthetic dense Q8 worker grammar; NO DEVICE')
