#!/usr/bin/env python3
"""Replay containment/input tests; never run the model or access a device."""
import array
import os
import pathlib
import shutil
import sys
import tempfile
from unittest import mock

import run_pioneer_q38_cpu_trace_rms_replay as replay
from compare_pioneer_q38_observed_trace import HIDDEN_WORDS, load_cpu

assert len(sys.argv) == 2
cpu_path = pathlib.Path(sys.argv[1])
assert cpu_path.is_absolute()
assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
cpu = load_cpu(cpu_path)
data = replay.authenticated_inputs(cpu_path)
words = array.array('I')
words.frombytes(data)
if sys.byteorder != 'little':
    words.byteswap()
assert list(words[:16]) == [0x31524d51, 1, 3, 65, 5120] + [0]*11
assert len(words) == 16+3*HIDDEN_WORDS
for position in range(3):
    assert words[16+position*HIDDEN_WORDS:16+(position+1)*HIDDEN_WORDS] == cpu[position]['bits'][:HIDDEN_WORDS]
refusals = 0


def refuses(call):
    global refusals
    try:
        call()
    except ValueError:
        refusals += 1
    else:
        raise AssertionError('invalid replay inputs accepted')


with mock.patch.object(replay, 'authenticated_inputs', side_effect=AssertionError('early refusal accessed CPU DATA')):
    for args in ([], ['/abs'], ['/abs', '/abs', '--work'], ['relative', '/abs'], ['/abs', 'relative']):
        refuses(lambda args=args: replay.main(args))
    for name in ('PIONEER_Q38_WORK_INFERENCE', 'PIONEER_Q38_TOKEN_NATIVE_CONTEXT', 'TT_PIONEER_Q38_WORK_MODE'):
        for value in ('0', '1'):
            with mock.patch.dict(os.environ, {name: value}):
                refuses(lambda: replay.main(['/abs', '/abs']))

with tempfile.TemporaryDirectory(prefix='pioneer-q38-rms-input-refusals-', dir='/private/tmp') as temporary:
    root = pathlib.Path(temporary)
    for position in range(3):
        shutil.copyfile(cpu_path/f'token-{position}.bin', root/f'token-{position}.bin')
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
print(f'PASS CPU_TRACE_RMS_REPLAY_INPUTS exact_original195_rows/998400_words/refusals{refusals}/invalid_role_or_control_refuses_before_compiler_and_model;NO device,native,Work or TPS')
