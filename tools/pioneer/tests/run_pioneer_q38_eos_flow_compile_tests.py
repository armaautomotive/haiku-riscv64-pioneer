#!/usr/bin/env python3
"""Compile-only original EOS observer integration; NOT execution qualification."""
import os
import pathlib
import re
import subprocess

assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
here = pathlib.Path(__file__).resolve().parent
subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-Wall', '-Wextra', '-Werror', '-fsyntax-only',
                str(here/'pioneer_q38_eos_state_data_bridge.cpp')], check=True)
flags = re.findall(r'-D[A-Z][A-Z0-9_]*(?:=[0-9]+)?', (here/'run_pioneer_q38_ffn_persistent_image_tests.sh').read_text())
for observed in (False, True):
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-Wall', '-Wextra', '-Werror', '-ffp-contract=off', '-fsyntax-only',
                    *flags, '-DPIONEER_Q38_TEST_CANCELLABLE_ROOT=1', *(['-DPIONEER_Q38_TEST_OBSERVED_TRACE=1'] if observed else []),
                    str(here/'pioneer_q38_token_original_flow_test.cpp')], check=True)
print('PASS original EOS integration compile/default observerOFF+exact observerON/raw-state-only callback after existing VerifyEosTokens source path;NOT executed EOS/native/Work/TPS proof')
