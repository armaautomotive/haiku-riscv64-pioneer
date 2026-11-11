#!/usr/bin/env python3
"""Compile-only identity and exclusion checks for the QGWE worker."""
import os
from pathlib import Path
import subprocess

header = Path(__file__).resolve().parent.parent / 'PioneerQ38DenseQ8MatrixWorkerLayout.h'
source = '#include <cstdint>\n#include "' + str(header) + '"\n'
source += 'static_assert(PioneerQ38DenseQ8MatrixWorkerLayout::Magic==0x45574751);\n'
source += 'static_assert(PioneerQ38DenseQ8MatrixWorkerLayout::JobWitness==0x454f4751);\n'
names = ('PIONEER_Q38_QGWE_BATCH_DENSE_WORKER', 'PIONEER_Q38_QGW8_BFP8_BATCH_WORKER',
         'PIONEER_Q38_QGWD_SRCA_DENSE_Q8_WORKER', 'PIONEER_Q38_DENSE_Q8_MATRIX_WORKER_INSPECTION',
         'PIONEER_Q38_DENSE_BF16_SRCA_BASE_CANDIDATE', 'PIONEER_Q38_HEAD_MATRIX_WORKER_INSPECTION',
         'PIONEER_Q38_HYBRID_WORKER_IMAGE', 'PIONEER_Q38_BF16_WORKER_IMAGE',
         'PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION', 'PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT')
flags = ['-D' + name + '=1' for name in names]

def compile_case(arguments, okay, body=source):
    result = subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', *arguments,
                             '-fsyntax-only', '-x', 'c++', '-'], input=body,
                            text=True, capture_output=True)
    assert (result.returncode == 0) == okay, result.stderr[:1000]

compile_case(flags, True)
for name in names:
    reduced = [flag for flag in flags if flag != '-D' + name + '=1']
    compile_case(reduced, False)
    compile_case(reduced + ['-D' + name + '=0'], False)
for name in ('_KERNEL_MODE', 'PIONEER_DRIVER_BUILD_TAG', 'TT_PIONEER_DRIVER_RELOAD',
             'PIONEER_Q38_TOKEN_NATIVE_CONTEXT', 'PIONEER_Q38_WORK_INFERENCE',
             'TT_PIONEER_Q38_WORK_MODE', 'PIONEER_Q38_CPU_VALIDATION',
             'PIONEER_Q38_FULL_MODEL_CPU_REFERENCE', 'PIONEER_Q38_QGW8_BFP8_REUSE_WORKER'):
    compile_case(flags + ['-D' + name + '=1'], False)
compile_case(flags, False, source + '#undef PIONEER_Q38_QGWE_BATCH_DENSE_WORKER\n#include "' + str(header) + '"\n')
print('PASS isolated QGWE identity and invalid-role refusals; compile-only, NO DEVICE OR SPEED')
