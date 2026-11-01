#!/usr/bin/env python3
"""Actual production fragments with synthetic state and MOCK transport,NO device IO."""
import os
import pathlib
import subprocess
import tempfile

assert not any(key.startswith(('PIONEER_', 'TT_PIONEER_')) for key in os.environ)
here = pathlib.Path(__file__).resolve().parent
head = (here.parent / 'blackhole_qwen38_global_head_control.inc').read_text()
feedback = (here.parent / 'blackhole_qwen38_global_feedback_control.inc').read_text()
token = (here.parent / 'blackhole_qwen38_token_head_control.inc').read_text()
fragments = {
    'INPUT': head[head.index('static void InputSeal(){'):head.index('static void Normalize(')],
    'FEEDBACK': feedback[feedback.index('static void FeedbackSeal('):feedback.index('static __attribute__((noinline)) void Feedback(')],
    'STATE': token[token.index('static __attribute__((noinline,section(".conv.token"))) void StateSeal('):
                   token.index('#ifdef PIONEER_Q38_TOKEN_FRONTIER_IMAGE')],
}
assert 'StateSeal(h,data,layer,feedback);' in fragments['STATE']
assert 'p->state[Slot]^=1;p->state[Committed]++;p->phase=6;HeadSeal(h);' in fragments['STATE']
with tempfile.TemporaryDirectory(prefix='pioneer-q38-commit-boundary-mock-', dir='/private/tmp') as temporary:
    out = pathlib.Path(temporary)
    defines = []
    for name, body in fragments.items():
        path = out / (name.lower() + '.inc')
        path.write_text(body)
        defines.append('-DPIONEER_Q38_COMMIT_' + name + '_FRAGMENT="' + str(path) + '"')
    binary = out / 'test'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=address,undefined', '-fno-omit-frame-pointer', *defines,
                    str(here / 'pioneer_q38_commit_boundary_test.cpp'), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True,
                   env={**os.environ, 'ASAN_OPTIONS': 'quarantine_size_mb=32', 'UBSAN_OPTIONS': 'halt_on_error=1'})
