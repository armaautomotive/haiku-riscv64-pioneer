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
binding_header = here.parent / 'PioneerQ38RecurrentQ8NativeBindingInspection.h'
compiler = os.environ.get('CXX', 'c++')
assert not any(name.startswith(('PIONEER_', 'TT_PIONEER_')) for name in os.environ)
with tempfile.TemporaryDirectory(prefix='q38-recurrent-receipt-', dir='/private/tmp') as tmp:
    binary = str(Path(tmp) / 'test')
    subprocess.run([compiler, '-std=c++17', '-O2', '-g', '-Wall', '-Wextra', '-Werror',
                    '-Wno-misleading-indentation', '-Wno-unused-function', '-fsanitize=address,undefined',
                    str(source), '-o', binary], check=True)
    stack = str(Path(tmp) / 'stack')
    subprocess.run([compiler, '-std=c++17', '-O2', '-fno-inline', '-fstack-usage', '-Wall', '-Wextra', '-Werror',
                    '-Wno-misleading-indentation', '-Wno-unused-function', str(source), '-o', stack], check=True)
    usage = list(Path(tmp).glob('*.su'))
    assert len(usage) == 1, usage
    rows = [line.split('\t') for line in usage[0].read_text().splitlines()]
    for marker, symbol in (('PioneerQ38RecurrentQ8UploadReceiptInspection.h:', 'Workspace'),
                           ('PioneerQ38RecurrentQ8MatrixWorkspace.h:', 'BuildWithScratch'),
                           ('PioneerQ38RecurrentQ8MatrixManifest.h:', 'BuildWithScratch')):
        matched = [row for row in rows if marker in row[0] and symbol in row[0]]
        assert len(matched) == 1 and len(matched[0]) == 3 and matched[0][2] == 'static' and int(matched[0][1]) <= 2048, (marker, matched)
    print('PASS receipt and scratch builders have host static frames <=2048; NOT full stack or kernel proof', flush=True)
    for args in ([], ['--abort'], ['--bad-record'], ['--invalidate'], ['--hole'], ['--rebind']):
        subprocess.run([binary, *args], check=True, env=dict(os.environ, UBSAN_OPTIONS='halt_on_error=1'))
    for forbidden in ('_KERNEL_MODE', 'PIONEER_DRIVER_BUILD_TAG', 'PIONEER_Q38_TOKEN_NATIVE_CONTEXT',
                      'PIONEER_Q38_WORK_INFERENCE', 'TT_PIONEER_Q38_WORK_MODE',
                      'PIONEER_Q38_CPU_VALIDATION', 'PIONEER_Q38_FULL_MODEL_CPU_REFERENCE'):
        result = subprocess.run([compiler, '-std=c++17', '-D' + forbidden + '=1', '-E', str(source)], capture_output=True)
        assert result.returncode and b'Recurrent upload receipt is offline inspection only' in result.stderr, forbidden
        assert b'Recurrent workspace transfer is offline inspection only' in result.stderr, forbidden
        assert b'Recurrent runtime plan is offline inspection only' in result.stderr, forbidden
        direct = subprocess.run([compiler, '-std=c++17', '-D' + forbidden + '=1',
                                 '-DPIONEER_Q38_RECURRENT_Q8_MATRIX_PLAN_INSPECTION=1',
                                 '-DPIONEER_Q38_RECURRENT_Q8_CANONICAL_INSPECTION=1', '-E', '-x', 'c++', '-'],
                                input='#include "' + str(binding_header) + '"\n', text=True, capture_output=True)
        assert direct.returncode and 'Recurrent native binding is offline inspection only' in direct.stderr, forbidden
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
    late_binding = subprocess.run([compiler, '-std=c++17', '-fsyntax-only', '-x', 'c++', '-'],
                                  input='#include "' + str(source) + '"\n'
                                        '#undef PIONEER_Q38_RECURRENT_Q8_MATRIX_PLAN_INSPECTION\n'
                                        '#include "' + str(binding_header) + '"\n', text=True, capture_output=True)
    assert late_binding.returncode and 'Recurrent native binding is offline inspection only' in late_binding.stderr
print('PASS recurrent receipt excludes native, Work and reference roles; NO DEVICE')
