#!/usr/bin/env python3
"""Synthetic recurrent upload ledger and role isolation; no device."""
import os
from pathlib import Path
import subprocess
import tempfile

here = Path(__file__).resolve().parent
source = here / 'pioneer_q38_recurrent_q8_upload_receipt_test.cpp'
header = here.parent / 'PioneerQ38RecurrentQ8UploadReceiptInspection.h'
transfer_header = here.parent / 'PioneerQ38RecurrentQ8WorkspaceTransferInspection.h'
runtime_header = here.parent / 'PioneerQ38RecurrentQ8RuntimePlanInspection.h'
compiler = os.environ.get('CXX', 'c++')
assert not any(name.startswith(('PIONEER_', 'TT_PIONEER_')) for name in os.environ)
with tempfile.TemporaryDirectory(prefix='q38-recurrent-receipt-', dir='/private/tmp') as tmp:
    binary = str(Path(tmp) / 'test')
    subprocess.run([compiler, '-std=c++17', '-O2', '-g', '-Wall', '-Wextra', '-Werror',
                    '-Wno-misleading-indentation', '-Wno-unused-function', '-fsanitize=address,undefined',
                    str(source), '-o', binary], check=True)
    for args in ([], ['--abort'], ['--bad-record'], ['--hole']):
        subprocess.run([binary, *args], check=True, env=dict(os.environ, UBSAN_OPTIONS='halt_on_error=1'))
    for forbidden in ('_KERNEL_MODE', 'PIONEER_DRIVER_BUILD_TAG', 'PIONEER_Q38_TOKEN_NATIVE_CONTEXT',
                      'PIONEER_Q38_WORK_INFERENCE', 'TT_PIONEER_Q38_WORK_MODE',
                      'PIONEER_Q38_CPU_VALIDATION', 'PIONEER_Q38_FULL_MODEL_CPU_REFERENCE'):
        result = subprocess.run([compiler, '-std=c++17', '-D' + forbidden + '=1', '-E', str(source)], capture_output=True)
        assert result.returncode and b'Recurrent upload receipt is offline inspection only' in result.stderr, forbidden
        assert b'Recurrent workspace transfer is offline inspection only' in result.stderr, forbidden
        assert b'Recurrent runtime plan is offline inspection only' in result.stderr, forbidden
    late = subprocess.run([compiler, '-std=c++17', '-fsyntax-only', '-x', 'c++', '-'],
                          input='#include "' + str(source) + '"\n'
                                '#undef PIONEER_Q38_RECURRENT_Q8_MATRIX_PLAN_INSPECTION\n'
                                '#include "' + str(header) + '"\n', text=True, capture_output=True)
    assert late.returncode and 'Recurrent upload receipt is offline inspection only' in late.stderr
    late_transfer = subprocess.run([compiler, '-std=c++17', '-fsyntax-only', '-x', 'c++', '-'],
                                   input='#include "' + str(source) + '"\n'
                                         '#undef PIONEER_Q38_RECURRENT_Q8_MATRIX_PLAN_INSPECTION\n'
                                         '#include "' + str(transfer_header) + '"\n', text=True, capture_output=True)
    assert late_transfer.returncode and 'Recurrent workspace transfer is offline inspection only' in late_transfer.stderr
    late_runtime = subprocess.run([compiler, '-std=c++17', '-fsyntax-only', '-x', 'c++', '-'],
                                  input='#include "' + str(source) + '"\n'
                                        '#undef PIONEER_Q38_RECURRENT_Q8_MATRIX_PLAN_INSPECTION\n'
                                        '#include "' + str(runtime_header) + '"\n', text=True, capture_output=True)
    assert late_runtime.returncode and 'Recurrent runtime plan is offline inspection only' in late_runtime.stderr
print('PASS recurrent receipt excludes native, Work and reference roles; NO DEVICE')
