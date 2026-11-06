#!/usr/bin/env python3
"""Compile roles and pure wire DATA; no image install or native grant."""
import os
import pathlib
import subprocess
import sys

here = pathlib.Path(__file__).resolve().parents[1]
assert len(sys.argv) == 2, 'usage: role-tests.py ABSOLUTE_HAIKU_SDK'
sdk = pathlib.Path(sys.argv[1])
compiler = sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku-g++'
assert sdk.is_absolute() and compiler.is_file()
accepted = refused = 0


def check(header, flags, okay, after='', body=''):
    global accepted, refused
    include = '#include "'+str(here/header)+'"\n'
    source = '#define PIONEER_ATTENTION_FREESTANDING\ntypedef __UINT32_TYPE__ uint32_t;\n'
    source += ''.join('#define '+name+' '+str(value)+'\n' for name, value in flags.items())
    source += include+after+include+body
    result = subprocess.run([str(compiler), '-std=c++17', '-Wall', '-Wextra', '-march=rv32imf', '-mabi=ilp32',
                             '-Werror', '-fsyntax-only', '-x', 'c++', '-'], input=source,
                            text=True, stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
    assert (result.returncode == 0) == okay, (header, flags, after, body, result.stderr)
    if okay:
        accepted += 1
    else:
        assert 'error:' in result.stderr
        refused += 1


bf = 'PIONEER_Q38_GLOBAL_CHAIN_BF16_FPU'
precise = 'PIONEER_Q38_GLOBAL_CHAIN_PRECISE_ACTIVATION'
data = 'PIONEER_Q38_TOKEN_BF16_PROGRAMS'
for parent in ({'PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE': 1, 'PIONEER_Q38_TOKEN_CANCEL_BOUNDARY': 1},
               {'PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE': 1,
                'PIONEER_Q38_GLOBAL_CHAIN_LOCAL_SOURCE_GUARDS': 1}):
    flags = {**parent, bf: 1, precise: 1}
    for header in ('PioneerQ38Bf16RootRole.h', 'PioneerQ38SelectedGlobalWorker.h'):
        check(header, flags, True)
        for name in flags:
            for value in (0, 2):
                check(header, {**flags, name: value}, False)
            check(header, {k: v for k, v in flags.items() if k != name},
                  header == 'PioneerQ38SelectedGlobalWorker.h' and name == bf)
            check(header, flags, False, '#undef '+name+'\n#define '+name+' 0\n')
        for name in ('_KERNEL_MODE', '__FAST_MATH__', 'PIONEER_DRIVER_BUILD_TAG',
                     'PIONEER_Q38_FULL_OFFLOAD_NATIVE', 'PIONEER_Q38_FULL_OFFLOAD_CLIENT',
                     'PIONEER_Q38_CPU_VALIDATION', 'PIONEER_Q38_FULL_MODEL_CPU_REFERENCE',
                     'PIONEER_Q38_WORK_INFERENCE', 'TT_PIONEER_Q38_WORK_MODE',
                     'PIONEER_Q38_BF16_WORKER_IMAGE', 'PIONEER_Q38_GLOBAL_CHAIN_PUSHED_COMPLETION',
                     'PIONEER_Q38_TOKEN_PRECISE_PROGRAMS', 'PIONEER_Q38_TOKEN_PUSHED_PROGRAMS', data):
            for value in (0, 1):
                check(header, {**flags, name: value}, False)
                check(header, flags, False, '#define '+name+' '+str(value)+'\n')

selection = 'PioneerQ38SelectedGlobalWorker.h'
for flags in ({data: 1}, {data: 1, '_KERNEL_MODE': 1}, {data: 1, 'PIONEER_Q38_FULL_OFFLOAD_CLIENT': 1}):
    check(selection, flags, True, body='static_assert(PioneerQ38SelectedWorkerBf16==1 && '
          'PioneerQ38SelectedGlobalWorker::Magic==0x34574751, "distinct QGW4 DATA");\n')
    check(selection, flags, False, body='void forbidden(){PioneerQ38Bf16Input::Pack(nullptr,256,nullptr,128);}\n')
for name in (precise, bf, 'PIONEER_Q38_TOKEN_PRECISE_PROGRAMS', 'PIONEER_Q38_TOKEN_PUSHED_PROGRAMS',
             'PIONEER_Q38_GLOBAL_CHAIN_PUSHED_COMPLETION'):
    for value in (0, 1):
        check(selection, {data: 1, name: value}, False)
for value in (0, 2):
    check(selection, {data: value}, False)
check(selection, {}, False, '#define '+data+' 1\n')
check(selection, {data: 1}, False, '#undef '+data+'\n')
check(selection, {precise: 1}, False, '#define '+bf+' 1\n')
check('PioneerQ38Bf16WorkerLayout.h', {'_KERNEL_MODE': 1}, True)
check('PioneerQ38Bf16Input.h', {'_KERNEL_MODE': 1}, False)
check('PioneerQ38Bf16Input.h', {}, True)
print(f'PASS BF16 ROOT/wire roles positive{accepted}/refusals{refused};kernel DATA without '
      'packer arithmetic,exact QGW4 BUILD and late selector separation;NOT native launch permission')
