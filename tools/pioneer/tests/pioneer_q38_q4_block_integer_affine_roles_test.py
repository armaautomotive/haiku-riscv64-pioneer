#!/usr/bin/env python3
"""Compile role gates only;no driver/device/source operations."""
import os
from pathlib import Path
import shlex
import subprocess

root = Path(__file__).resolve().parents[1]
selection = '#define PIONEER_Q38_Q4_BLOCK_INTEGER_AFFINE_OFFBOARD 1\n'
body = ('struct E { void Op(unsigned) {} void Config(unsigned,unsigned) {} void Drain() {} };\n'
        'int main(){E e;PioneerQ38Q4BlockIntegerAffine::Panel<0>(e);'
        'PioneerQ38Q4BlockIntegerAffine::Panel<2>(e);'
        'PioneerQ38Q4BlockIntegerAffine::Finish<0>(e);'
        'PioneerQ38Q4BlockIntegerAffine::Finish<2>(e);}\n')
compiler = shlex.split(os.environ.get('CXX', 'c++'))


def check(source, success, extra=()):
    result = subprocess.run(compiler + ['-std=c++17', '-Wall', '-Wextra', '-Werror',
                            '-fsyntax-only', '-I', str(root), '-x', 'c++', '-'] + list(extra),
                            input=source, text=True, capture_output=True, check=False)
    assert (result.returncode == 0) == success, result.stderr
    if success:
        assert not result.stderr and not result.stdout


refusals = positives = 0
for name in ('PioneerQ38Q4BlockIntegerAffine.h', 'PioneerQ38Q4BlockIntegerState.h'):
    header = f'#include "{name}"\n'
    selected_body = body
    if name.endswith('State.h'):
        selected_body = body.replace('int main(){E e;',
                                    'int main(){E e;PioneerQ38Q4BlockIntegerState s;'
                                    's.Init(e);s.Begin(e,0x98000);s.Panel(e,0,0);s.Finish(e,0x3f800000);')
    check(selection + header + selected_body, True)
    positives += 1
    for value in (None, 0, 2):
        prefix = '' if value is None else f'#define PIONEER_Q38_Q4_BLOCK_INTEGER_AFFINE_OFFBOARD {value}\n'
        check(prefix + header + selected_body, False)
        refusals += 1
    for forbidden in ('_KERNEL_MODE', 'PIONEER_ATTENTION_FREESTANDING',
                      'PIONEER_Q38_WORK_INFERENCE', 'PIONEER_Q38_CPU_VALIDATION',
                      'PIONEER_Q38_FULL_MODEL_CPU_REFERENCE', 'PIONEER_Q38_FULL_OFFLOAD_NATIVE',
                      'PIONEER_Q38_FULL_OFFLOAD_CLIENT', 'PIONEER_Q38_QUEUED_Q4_BLOCK_IMAGE',
                      '__FAST_MATH__'):
        for value in (0, 1):
            define = f'#define {forbidden} {value}\n'
            check(selection + define + header + selected_body, False)
            check(selection + header + define + header + selected_body, False)
            refusals += 2
    for parity in (1, 3):
        check(selection + header + selected_body.replace('Panel<0>', f'Panel<{parity}>'), False)
        check(selection + header + selected_body.replace('Finish<0>', f'Finish<{parity}>'), False)
        refusals += 2
    check(selection + header + selected_body, False, ('-ffast-math',))
    refusals += 1
print(f'PASS block-integer arithmetic+state roles{positives}positives/{refusals}refusals;missing/zero/two/late/native/client/Work/reference/fastmath/parity;NO_HARDWARE')
