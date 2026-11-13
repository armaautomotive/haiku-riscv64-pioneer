#!/usr/bin/env python3
"""Build isolated small recurrent cache checker; optionally test real data."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile

if len(sys.argv) not in (1, 3) or any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ):
    raise SystemExit('usage: [absolute-model absolute-existing-cache]')
here = Path(__file__).resolve().parent
source = here / 'pioneer_q38_small_recurrent_real_cache_check.cpp'
role = '-DPIONEER_Q38_REAL_SMALL_RECURRENT_CACHE_CHECK=1'
common = [os.environ.get('CXX', 'c++'), '-std=c++17', '-O2', '-Wall', '-Wextra', '-Werror',
          '-Wno-misleading-indentation', '-ffp-contract=off']
for flags in ([], ['-DPIONEER_Q38_REAL_SMALL_RECURRENT_CACHE_CHECK=0'],
              *([role, '-D' + name + '=1'] for name in ('_KERNEL_MODE', 'PIONEER_DRIVER_BUILD_TAG',
                 'PIONEER_Q38_TOKEN_NATIVE_CONTEXT', 'PIONEER_Q38_WORK_INFERENCE', 'TT_PIONEER_Q38_WORK_MODE',
                 'PIONEER_Q38_CPU_VALIDATION', 'PIONEER_Q38_FULL_MODEL_CPU_REFERENCE', '__FAST_MATH__'))):
    result = subprocess.run([*common, *flags, '-fsyntax-only', str(source)], capture_output=True, text=True)
    assert result.returncode and 'isolated offline DATA only' in result.stderr, flags
with tempfile.TemporaryDirectory(prefix='q38-small-real-check-', dir='/private/tmp') as directory:
    tmp = Path(directory); binary = tmp / 'check'; dummy = tmp / 'dummy'
    dummy.write_bytes(b'not a model')
    subprocess.run([*common, role, str(source), str(here.parent / 'PioneerModelWeightCache.cpp'), '-o', str(binary)], check=True)
    for args in ([], ['--wrong', str(dummy), str(tmp)], ['--verify-small-recurrent-bfp8', 'relative', str(tmp)],
                 ['--verify-small-recurrent-bfp8', str(dummy), 'relative']):
        assert subprocess.run([str(binary), *args], capture_output=True).returncode == 2
    assert subprocess.run([str(binary), '--verify-small-recurrent-bfp8', str(dummy), str(tmp)], capture_output=True).returncode == 1
    print('PASS small recurrent checker role and invalid-input refusals; NO DEVICE', flush=True)
    if len(sys.argv) == 3:
        model, cache = map(Path, sys.argv[1:])
        assert model.is_absolute() and cache.is_absolute() and cache.is_dir()
        subprocess.run([str(binary), '--verify-small-recurrent-bfp8', str(model), str(cache)], check=True)
