#!/usr/bin/env python3
"""Compile-only TEST transport boundaries; NO file authentication/device IO."""
import os
import pathlib
import subprocess

assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
here = pathlib.Path(__file__).resolve().parent
compiler = os.environ.get('CXX', 'c++')
bridge = 'pioneer_q38_embedding_raw_test_bridge.h'
role = 'PIONEER_Q38_EMBEDDING_RAW_TEST_BRIDGE'
fixture = 'PIONEER_Q38_TOKEN_NATIVE_EMBEDDING_FIXTURE'
context = 'PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY'
passed = refused = 0


def check(header, defines, success, implementation=False):
    global passed, refused
    args = [compiler, '-std=c++17', '-Wall', '-Wextra', '-Werror', '-fsyntax-only', '-I'+str(here)]
    args += ['-D'+key+'='+str(value) for key, value in defines.items()]
    if implementation:
        args += [str(here/header)]
        source = None
    else:
        args += ['-x', 'c++', '-']
        source = '#include "'+header+'"\n'
    result = subprocess.run(args, input=source, text=True, capture_output=True)
    assert (result.returncode == 0) == success, (header, defines, result.stderr)
    if success:
        passed += 1
    else:
        assert 'error:' in result.stderr
        refused += 1


check(bridge, {role:1}, True)
check(bridge, {role:1, '_KERNEL_MODE':1, fixture:1, context:1}, True)  # interface only in explicitly emulated fixture
for defs in ({}, {role:0}, {role:2}, {role:1, '_KERNEL_MODE':1},
             {role:1, '_KERNEL_MODE':1, fixture:1}, {role:1, '_KERNEL_MODE':1, context:1},
             {role:1, '_KERNEL_MODE':1, fixture:0, context:1},
             {role:1, '_KERNEL_MODE':1, fixture:1, context:2}):
    check(bridge, defs, False)
for macro in ('PIONEER_Q38_WORK_INFERENCE', 'PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE',
              'PIONEER_Q38_GLOBAL_WORKER_IMAGE', 'PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE'):
    for value in (0, 1):
        check(bridge, {role:1, macro:value}, False)
for macro in ('_KERNEL_MODE', 'PIONEER_Q38_TOKEN_NATIVE_CONTEXT', 'PIONEER_Q38_WORK_INFERENCE'):
    for value in (0, 1):
        check('pioneer_q38_embedding_raw_test_bridge.cpp', {macro:value}, False, True)
for header in ('pioneer_q38_whole_original_bytes.h', 'pioneer_q38_original_rows.h'):
    for value in (0, 1):
        check(header, {'_KERNEL_MODE':value}, False)  # ORIGINAL strict guards unchanged
print(f'PASS opaque RAW TEST interface/{passed} explicit host+emulated-interface roles/{refused} absent+zero+two+kernel+device+Work refusals/host implementation rejects ALL native roles/original transport guards UNCHANGED;NO native permission or products')
