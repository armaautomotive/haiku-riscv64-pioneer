#!/usr/bin/env python3
"""Compile and exercise the isolated routed-token image/footprint DATA."""
import os
import pathlib
import subprocess
import tempfile

if any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ):
    raise SystemExit('refusing Pioneer role overrides')
here = pathlib.Path(__file__).resolve().parent
with tempfile.TemporaryDirectory(prefix='pioneer-routed-token-programs-', dir='/private/tmp') as tmp:
    binary = pathlib.Path(tmp) / 'test'
    compiler = os.environ.get('CXX', 'c++')
    subprocess.run([compiler, '-std=c++17', '-O2', '-Wall', '-Wextra', '-Werror',
                    '-DPIONEER_Q38_TOKEN_REDISTRIBUTED_GDN_PROGRAMS=1',
                    str(here/'pioneer_q38_routed_token_programs_test.cpp'), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
    native = subprocess.run([compiler, '-std=c++17', '-fsyntax-only', '-I'+str(here.parent),
                             '-DPIONEER_Q38_TOKEN_REDISTRIBUTED_GDN_PROGRAMS=1',
                             '-include', str(here.parent/'PioneerQ38TokenSelectedPrograms.h'), '-x', 'c++', '-'],
                            input='int main() { return 0; }\n', text=True, capture_output=True)
    assert native.returncode != 0 and 'DATA-only; native token selection/launch is not authorized' in native.stderr
    print('PASS routed image DATA selector is compile-time rejected by native token dispatcher')
