#!/usr/bin/env python3
"""Build and test the separate CPU QGW8 head observer; no real model run."""
import os
from pathlib import Path
import subprocess
import tempfile

here = Path(__file__).resolve().parent
tools = here.parent
cxx = os.environ.get('CXX', 'c++')
common = [cxx, '-std=c++17', '-O2', '-Wall', '-Wextra', '-Werror', '-ffp-contract=off']
backend = str(here / 'pioneer_q38_head_bfp8_cpu_backend.cpp')
main = str(here / 'pioneer_q38_qgw8_cpu_head_compare.cpp')

with tempfile.TemporaryDirectory(prefix='pioneer-qgw8-cpu-head-tests-') as tmp:
    unit = str(Path(tmp) / 'unit')
    subprocess.run(common + ['-Wno-unused-function', '-fsanitize=address,undefined',
        str(here / 'pioneer_q38_head_bfp8_cpu_project_test.cpp'), '-o', unit], check=True)
    subprocess.run([unit], check=True)
    snapshot = str(Path(tmp) / 'snapshot-test')
    subprocess.run(common + ['-fsanitize=address,undefined',
        str(here / 'pioneer_q38_qgw8_cpu_hidden_snapshot_test.cpp'), '-o', snapshot], check=True)
    subprocess.run([snapshot, tmp], check=True)
    layers = str(Path(tmp) / 'layer-snapshot-test')
    subprocess.run(common + ['-fsanitize=address,undefined',
        str(here / 'pioneer_q38_qgw8_cpu_layer_snapshot_test.cpp'), '-o', layers], check=True)
    subprocess.run([layers, tmp], check=True)
    subprocess.run(['python3', str(here / 'test_pioneer_q38_qgw8_cpu_hidden_snapshot.py')], check=True)
    snapshot_header = str(here / 'pioneer_q38_qgw8_cpu_hidden_snapshot.h')
    for role in ('_KERNEL_MODE', 'PIONEER_Q38_WORK_INFERENCE', 'TT_PIONEER_Q38_WORK_MODE', '__FAST_MATH__'):
        result = subprocess.run(common + ['-DPIONEER_Q38_BFP4_QUALITY_DIAGNOSTIC=1', '-D' + role,
                                          '-fsyntax-only', '-x', 'c++', snapshot_header],
                                stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        if result.returncode == 0 or 'isolated reference data only' not in result.stderr:
            raise RuntimeError('Hidden snapshot role guard failed: ' + role)
    result = subprocess.run(common + ['-fsyntax-only', '-x', 'c++', snapshot_header],
                            stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    if result.returncode == 0 or 'isolated reference data only' not in result.stderr:
        raise RuntimeError('Hidden snapshot accepted without diagnostic role')
    executable = str(Path(tmp) / 'compare')
    subprocess.run(common + ['-DPIONEER_Q38_BFP4_QUALITY_DIAGNOSTIC=1', main,
        str(here / 'pioneer_q38_bfp4_full_cpu_backend.cpp'), backend,
        str(here / 'pioneer_q38_cpu_model_reference.cpp'), str(here / 'pioneer_q38_whole_original_bytes.cpp'),
        str(tools / 'PioneerModelWeightCache.cpp'), '-o', executable], check=True)
    for args in ([], ['--invalid', '/not-a-model'], ['--fixed-one-qgw8-head', 'relative'],
                 ['--fixed-one-qgw8-head', '/not-a-model', '-1'],
                 ['--fixed-one-qgw8-head', '/not-a-model', '248320'],
                 ['--fixed-one-qgw8-head', '/not-a-model', '1234567'],
                 ['--fixed-one-qgw8-head', '/not-a-model', '1x'],
                 ['--fixed-one-qgw8-head', '/not-a-model', '108133', '--wrong', '/tmp/new'],
                 ['--fixed-one-qgw8-head', '/not-a-model', '108133', '--save-converted-layers', 'relative'],
                 ['--fixed-one-qgw8-head', '/not-a-model', '108133', '--save-hidden', 'relative']):
        result = subprocess.run([executable, *args], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        if result.returncode != 2:
            raise RuntimeError('Argument guard failed: ' + repr(args))
    for role in ('_KERNEL_MODE', 'PIONEER_Q38_WORK_INFERENCE', 'TT_PIONEER_Q38_WORK_MODE',
                 'PIONEER_Q38_CPU_VALIDATION', 'PIONEER_Q38_FULL_MODEL_CPU_REFERENCE'):
        result = subprocess.run(common + ['-D' + role, '-fsyntax-only', backend],
                                stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        if result.returncode == 0 or 'separately compiled CPU-only diagnostic' not in result.stderr:
            raise RuntimeError('Backend role guard failed: ' + role)
    subprocess.run(['python3', str(here / 'pioneer_q38_qgw8_head_capture_test.py')], check=True)

print('PASS QGW8 head observer build, synthetic arithmetic, atomic refusals and CPU-only roles; no model/device/TPS')
