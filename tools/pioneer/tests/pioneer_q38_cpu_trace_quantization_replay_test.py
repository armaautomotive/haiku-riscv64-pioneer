#!/usr/bin/env python3
"""Early containment checks; no model,device,compiler or native execution."""
import os
import pathlib
import shutil
import sys
import tempfile
from unittest import mock

import run_pioneer_q38_cpu_trace_quantization_replay as replay

assert len(sys.argv) == 2
cpu = pathlib.Path(sys.argv[1])
assert cpu.is_absolute()
assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
refusals = 0


def refuses(call):
    global refusals
    try:
        call()
    except ValueError:
        refusals += 1
    else:
        raise AssertionError('invalid CPU-input quantization replay accepted')


with mock.patch.object(replay, 'authenticated_inputs', side_effect=AssertionError('early refusal accessed CPU inputs')):
    for args in ([], ['/abs'], ['/abs', '/abs', '--work'], ['relative', '/abs'], ['/abs', 'relative']):
        refuses(lambda args=args: replay.main(args))
    for name in ('PIONEER_Q38_WORK_INFERENCE', 'PIONEER_Q38_TOKEN_NATIVE_CONTEXT', 'TT_PIONEER_Q38_WORK_MODE'):
        for value in ('0', '1'):
            with mock.patch.dict(os.environ, {name: value}):
                refuses(lambda: replay.main(['/abs', '/abs']))

with tempfile.TemporaryDirectory(prefix='pioneer-q38-quantization-refusals-', dir='/private/tmp') as temporary:
    root = pathlib.Path(temporary)
    for position in range(3):
        shutil.copyfile(cpu/f'token-{position}.bin', root/f'token-{position}.bin')
    with mock.patch.object(replay.subprocess, 'run', side_effect=AssertionError('bad control reached compiler/model')):
        for position in range(3):
            path = root/f'token-{position}.bin'
            original = path.read_bytes()
            for offset in (0, 64, len(original)-1):
                changed = bytearray(original)
                changed[offset] ^= 1
                path.write_bytes(changed)
                refuses(lambda: replay.main([str(root/'NO_MODEL'), str(root)]))
            path.write_bytes(original[:-4])
            refuses(lambda: replay.main([str(root/'NO_MODEL'), str(root)]))
            path.write_bytes(original)
print(f'PASS CPU_TRACE_QUANTIZATION_REPLAY_INPUTS refusals{refusals}/invalid_role_or_control_refuses_before_compiler_and_model;NO device,native,Work or TPS')
