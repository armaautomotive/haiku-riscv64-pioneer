#!/usr/bin/env python3
"""Build the separate CPU-only replay and test strict synthetic input guards."""
import os
from pathlib import Path
import subprocess
import tempfile

here = Path(__file__).resolve().parent
tools = here.parent
cxx = os.environ.get('CXX', 'c++')
common = [cxx, '-std=c++17', '-O2', '-Wall', '-Wextra', '-Werror', '-Wno-misleading-indentation', '-ffp-contract=off']
role = '-DPIONEER_Q38_CARD_HEAD_REPLAY=1'
source = str(here / 'pioneer_q38_qgw8_card_head_replay.cpp')
header = str(here / 'pioneer_q38_qgw8_card_hidden_data.h')
for invalid in ('_KERNEL_MODE', 'PIONEER_Q38_WORK_INFERENCE', 'TT_PIONEER_Q38_WORK_MODE', '__FAST_MATH__'):
    result = subprocess.run(common + [role, '-D' + invalid, '-fsyntax-only', '-x', 'c++', header], capture_output=True)
    assert result.returncode, invalid
result = subprocess.run(common + ['-fsyntax-only', '-x', 'c++', header], capture_output=True)
assert result.returncode
with tempfile.TemporaryDirectory(prefix='qgw8-card-head-tests-', dir='/private/tmp') as name:
    unit, binary = str(Path(name) / 'unit'), str(Path(name) / 'replay')
    subprocess.run(common + ['-fsanitize=address,undefined', str(here / 'pioneer_q38_qgw8_card_hidden_data_test.cpp'),
                             str(tools / 'PioneerModelWeightCache.cpp'), '-o', unit], check=True)
    subprocess.run([unit], check=True)
    subprocess.run(common + [role, source, str(here / 'pioneer_q38_head_bfp8_cpu_backend.cpp'),
        str(here / 'pioneer_q38_whole_original_bytes.cpp'), str(tools / 'PioneerModelWeightCache.cpp'), '-o', binary], check=True)
    for args in ([], ['--bad', '/model', '/input', 'x', '0'], ['--card-hidden-head', 'relative', '/input', 'x', '0'],
                 ['--card-hidden-head', '/model', '/input', 'x', '-1'],
                 ['--card-hidden-head', '/model', '/input', 'x', '248320']):
        result = subprocess.run([binary, *args], capture_output=True)
        assert result.returncode == 2, (args, result.returncode, result.stderr)
    result = subprocess.run([binary, '--card-hidden-head', '/model', '/input', '0'*64, '108133'], capture_output=True)
    assert result.returncode == 1 and b'CARD_HIDDEN_INVALID' in result.stderr
print('PASS isolated CPU head-replay build, role and input guards; no real model or card')
