#!/usr/bin/env python3
"""Distinct image role syntax checks;never a firmware execution grant."""
import os
from pathlib import Path
import shlex
import subprocess

root = Path(__file__).resolve().parents[1]
required = ('PIONEER_Q38_Q4_BLOCK_INTEGER_IMAGE', 'PIONEER_Q38_Q4_BLOCK_INTEGER_AFFINE_OFFBOARD',
            'PIONEER_Q38_PREPARED_EXACT_Q4_IMAGE', 'PIONEER_Q38_PREPARED_TWO_PRODUCT_IMAGE',
            'PIONEER_Q38_PREPARED_TWO_PRODUCT_REPLAY_IMAGE', 'PIONEER_Q38_EXACT_Q4_WORK_IMAGE',
            'PIONEER_Q38_VECTOR_Q4_METADATA_IMAGE', 'PIONEER_Q38_VECTOR_Q4_METADATA',
            'PIONEER_Q38_TOKEN_TWIN_BATCH_PREFETCH_WORKER_IMAGE', 'PIONEER_Q38_WORK_TWIN_BATCH_PREFETCH')
forbidden = ('_KERNEL_MODE', 'PIONEER_DRIVER_BUILD_TAG', 'TT_PIONEER_DRIVER_RELOAD',
             'PIONEER_Q38_CPU_VALIDATION', 'PIONEER_Q38_FULL_MODEL_CPU_REFERENCE',
             'PIONEER_Q38_WORK_INFERENCE', 'TT_PIONEER_Q38_WORK_MODE',
             'PIONEER_Q38_FULL_OFFLOAD_NATIVE', 'PIONEER_Q38_FULL_OFFLOAD_CLIENT',
             'PIONEER_Q38_QUEUED_Q4_BLOCK_IMAGE', 'PIONEER_Q38_QUEUED_Q4_FIXED_CONFIG_IMAGE',
             'PIONEER_Q38_QUEUED_Q4_PAIR_CONFIG_IMAGE', 'PIONEER_Q38_QUEUED_Q4_DELTA_CONFIG_IMAGE', '__FAST_MATH__')
defines = ''.join(f'#define {name} 1\n' for name in required)
header = ('#include "PioneerQ38Q4BlockIntegerImageRole.h"\n'
          '#include "PioneerQ38Q4BlockIntegerState.h"\n')
body = ('struct E {void Op(unsigned) {} void Config(unsigned,unsigned) {} void Drain() {}};'
        'int main(){E e;PioneerQ38Q4BlockIntegerState s;s.Init(e);s.Begin(e,0x98000);'
        's.Panel(e,0,0);s.Finish(e,0x3f800000);}\n')
compiler = shlex.split(os.environ.get('CXX', 'c++'))


def check(source, success):
    result = subprocess.run(compiler + ['-std=c++11', '-Wall', '-Wextra', '-Werror', '-fsyntax-only',
                            '-I', str(root), '-x', 'c++', '-'], input='#include <stdint.h>\n' + source,
                            text=True, capture_output=True, check=False)
    assert (result.returncode == 0) == success, result.stderr
    if success:
        assert not result.stdout and not result.stderr


check(defines + header + body, True)
check(defines + '#define PIONEER_ATTENTION_FREESTANDING\n' + header + body, True)
refusals = 0
for name in required:
    for value in (None, 0, 2):
        changed = defines.replace(f'#define {name} 1\n', '' if value is None else f'#define {name} {value}\n')
        check(changed + header + body, False)
        refusals += 1
    check(defines + header + f'#undef {name}\n#define {name} 0\n' + header + body, False)
    refusals += 1
for name in forbidden:
    for value in (0, 1):
        define = f'#define {name} {value}\n'
        check(defines + define + header + body, False)
        check(defines + header + define + header + body, False)
        refusals += 2
print(f'PASS block-integer complete image roles2positives/{refusals}refusals;exactParents_CXX11_hostOrFreestanding_late_native_client_Work_reference_queuedSiblings_definedZero;NO_HARDWARE')
