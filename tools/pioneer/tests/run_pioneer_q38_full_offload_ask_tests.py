#!/usr/bin/env python3
"""Real prompt client IPC/process tests against FAKE peers;never hardware."""
import os
import pathlib
import subprocess
import tempfile

here = pathlib.Path(__file__).resolve().parent
project = here.parent.parent.parent
source = project / 'src/apps/assistant/P100AQwen27FullOffload.cpp'
common = [os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
          '-fsanitize=address,undefined', '-DPIONEER_Q38_FULL_OFFLOAD_PROMPT_CLIENT=1']
with tempfile.TemporaryDirectory(prefix='pioneer-full-ask-tests-', dir='/private/tmp') as temporary:
    out = pathlib.Path(temporary)
    worker, client, cli = (out / n for n in ('fake', 'client', 'ask'))
    subprocess.run([*common, str(here / 'pioneer_q38_full_offload_ask_fake.cpp'), '-o', str(worker)], check=True)
    define = '-DASSISTANT_QWEN27_FULL_OFFLOAD_WORKER="' + str(worker) + '"'
    for main, binary in ((here / 'pioneer_q38_full_offload_ask_test.cpp', client),
                         (here.parent / 'pioneer_q38_full_offload_ask.cpp', cli)):
        subprocess.run([*common, define, str(main), str(source), '-o', str(binary)], check=True)
    for mode in range(35):
        trace = out / ('trace-' + str(mode))
        env = {**os.environ, 'ASK_TEST_CASE': str(mode), 'ASK_TEST_TRACE': str(trace),
               'UBSAN_OPTIONS': 'halt_on_error=1'}
        result = subprocess.run([str(client)], capture_output=True, text=True, timeout=15, env=env)
        assert result.returncode == 0 and not result.stderr, (mode, result.returncode, result.stdout, result.stderr)
        if mode in (32, 33):
            assert not trace.exists(), mode  # rejected/cancelled BEFORE even spawning
        else:
            rows = trace.read_text().splitlines()
            assert rows[0] == 'read 1 1 1', (mode, rows)
            assert not any(r.startswith('read 5 ') for r in rows)  # never configure CPU threads
            if mode not in (0, 1, 2, 3, 19, 20, 21, 22, 23, 25, 30, 31):
                assert 'read 4 3 0' not in rows, (mode, rows)  # unknown=close only,not implicit END
            assert sum(r == 'read 3 2 0' for r in rows) == int(mode in (30, 31)), (mode, rows)
            if mode == 34:
                assert rows == ['read 1 1 1', 'read 4 2 0'], rows  # cancel AFTER Load,BEFORE Generate
    args = ['--full-card-question', 'What is 2 + 2?', '--tokens', '4']
    env = {**os.environ, 'ASK_TEST_CASE': '0', 'ASK_TEST_TRACE': str(out / 'cli-trace')}
    result = subprocess.run([str(cli), *args], capture_output=True, text=True, timeout=15, env=env)
    assert result.returncode == 0 and not result.stderr and 'hex=34\n' in result.stdout, result
    assert result.stdout.endswith('PASS_FULL_CARD_ASK clean_worker_exit1 CPU_INFERENCE_CONTEXTS0;NO_FALLBACK;NOT_BROAD_QUALITY_OR_SPEED_PROOF\n')
    bad = [[], ['--full-card-rate-32'], [*args, 'extra'], ['--full-card-question', '', '--tokens', '4'],
           ['--full-card-question', 'a' * 257, '--tokens', '4']]
    for number in ('0', '00', '04', '-1', '+4', '4x', '33', '999', ' 4', '4 ', ''):
        bad.append(['--full-card-question', 'What is 2 + 2?', '--tokens', number])
    for arguments in bad:
        result = subprocess.run([str(cli), *arguments], capture_output=True, text=True, timeout=15)
        assert result.returncode == 2 and not result.stdout and not result.stderr, (arguments, result)
    for extra in (['-UPIONEER_Q38_FULL_OFFLOAD_PROMPT_CLIENT'],
                  ['-UPIONEER_Q38_FULL_OFFLOAD_PROMPT_CLIENT', '-DPIONEER_Q38_FULL_OFFLOAD_PROMPT_CLIENT=0'],
                  ['-ffast-math'], *[['-D' + m + '=0'] for m in
                   ('_KERNEL_MODE', 'PIONEER_Q38_CPU_VALIDATION', 'PIONEER_Q38_FULL_MODEL_CPU_REFERENCE',
                    'PIONEER_Q38_WORK_INFERENCE', 'TT_PIONEER_Q38_WORK_MODE')]):
        result = subprocess.run([*common, *extra, '-fsyntax-only', str(source)], capture_output=True, text=True)
        assert result.returncode and 'Full-card prompt client requires' in result.stderr, extra
print('PASS one-shot full-card prompt adapter/CLI;35 FAKE channel/process cases,16 CLI refusals,8 role refusals;'
      'known clean worker+actual exit before answer;unknown close only;NO device/model/numerical/speed or installation result')
