#!/usr/bin/env python3
"""Build and guard the real-FFN-BFP8 cache checker without opening real data."""
import os
from pathlib import Path
import subprocess
import tempfile

assert not any(key.startswith(('PIONEER_', 'TT_PIONEER_')) for key in os.environ)
here = Path(__file__).resolve().parent
source = str(here / 'pioneer_q38_qgw8_real_ffn_bfp8_cache_check.cpp')
compiler = os.environ.get('CXX', 'c++')
common = [compiler, '-std=c++17', '-Wall', '-Wextra', '-Werror', '-Wno-misleading-indentation', '-ffp-contract=off']
role = '-DPIONEER_Q38_REAL_FFN_BFP8_CACHE_CHECK=1'
for flags in ([], ['-DPIONEER_Q38_REAL_FFN_BFP8_CACHE_CHECK=0'],
              *([role, '-D' + key + '=1'] for key in ('_KERNEL_MODE', 'PIONEER_Q38_WORK_INFERENCE',
                   'TT_PIONEER_Q38_WORK_MODE', 'PIONEER_Q38_CPU_VALIDATION', 'PIONEER_Q38_FULL_MODEL_CPU_REFERENCE', '__FAST_MATH__'))):
    result = subprocess.run(common + flags + ['-fsyntax-only', source], capture_output=True, text=True)
    assert result.returncode and 'Real FFN BFP8 cache check is separate offline diagnostic DATA only' in result.stderr, flags
for key in ('PIONEER_DRIVER_BUILD_TAG', 'PIONEER_Q38_TOKEN_NATIVE_CONTEXT',
            'PIONEER_Q38_HYBRID_FULL_NATIVE', 'PIONEER_Q38_FULL_OFFLOAD_NATIVE', 'PIONEER_Q38_FULL_OFFLOAD_CLIENT'):
    assert subprocess.run(common + [role, '-D' + key + '=1', '-fsyntax-only', source], capture_output=True).returncode, key
with tempfile.TemporaryDirectory(prefix='qgw8-real-FFN-BFP8-check-', dir='/private/tmp') as name:
    tmp = Path(name)
    binary, dummy = tmp / 'check', tmp / 'dummy'
    dummy.write_bytes(b'not a model')
    subprocess.run(common + ['-O2', role, source, str(here.parent / 'PioneerModelWeightCache.cpp'), '-o', str(binary)], check=True)
    for args in ([], ['--wrong', str(dummy), str(tmp)], ['--verify-cached-ffn-bfp8', 'relative', str(tmp)],
                 ['--verify-cached-ffn-bfp8', str(dummy), 'relative']):
        assert subprocess.run([str(binary), *args], capture_output=True).returncode == 2, args
    result = subprocess.run([str(binary), '--verify-cached-ffn-bfp8', str(dummy), str(tmp)], capture_output=True, text=True)
    assert result.returncode == 1 and 'authenticating real model' not in result.stdout
    assert dummy.read_bytes() == b'not a model'
print('PASS offline real-FFN-BFP8 checker build, role and invalid-input refusals; NO real model, cache writes or device')
