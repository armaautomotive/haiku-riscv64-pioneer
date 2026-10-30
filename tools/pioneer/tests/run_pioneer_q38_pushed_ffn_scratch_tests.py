#!/usr/bin/env python3
"""Check selected FFN scratch lifetimes in HOST DATA profiles, never device IO."""
import os
import pathlib
import subprocess
import tempfile

assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
here = pathlib.Path(__file__).resolve().parent
profiles = ((), ('PIONEER_Q38_TOKEN_PRECISE_PROGRAMS=1',),
            ('PIONEER_Q38_GLOBAL_CHAIN_PRECISE_ACTIVATION=1',),
            ('PIONEER_Q38_GLOBAL_CHAIN_PRECISE_ACTIVATION=1', 'PIONEER_Q38_GLOBAL_CHAIN_PUSHED_COMPLETION=1'),
            ('PIONEER_Q38_TOKEN_PUSHED_PROGRAMS=1',))
with tempfile.TemporaryDirectory(prefix='pioneer-q38-pushed-ffn-scratch-', dir='/private/tmp') as temporary:
    for index, flags in enumerate(profiles):
        binary = pathlib.Path(temporary)/str(index)
        subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                        '-fsanitize=address,undefined', *['-D'+flag for flag in flags],
                        str(here/'pioneer_q38_pushed_ffn_scratch_test.cpp'), '-o', str(binary)], check=True)
        subprocess.run([str(binary)], check=True, env={**os.environ, 'UBSAN_OPTIONS':'halt_on_error=1'})
    refused = (('PIONEER_Q38_TOKEN_PUSHED_PROGRAMS=0',),
               ('PIONEER_Q38_TOKEN_PUSHED_PROGRAMS=1', 'PIONEER_Q38_TOKEN_PRECISE_PROGRAMS=1'),
               ('PIONEER_Q38_GLOBAL_CHAIN_PUSHED_COMPLETION=0', 'PIONEER_Q38_GLOBAL_CHAIN_PRECISE_ACTIVATION=1'),
               ('PIONEER_Q38_GLOBAL_CHAIN_PUSHED_COMPLETION=1',),
               ('PIONEER_Q38_TOKEN_PUSHED_PROGRAMS=1', 'PIONEER_Q38_GLOBAL_CHAIN_PUSHED_COMPLETION=1', 'PIONEER_Q38_GLOBAL_CHAIN_PRECISE_ACTIVATION=1'))
    for flags in refused:
        result = subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-fsyntax-only',
                                 *['-D'+flag for flag in flags], str(here/'pioneer_q38_pushed_ffn_scratch_test.cpp')],
                                stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        assert result.returncode and 'error:' in result.stderr, flags
    print('PASS FFN scratch DATA selection/five positive profiles/five invalid or mixed profiles refused')
