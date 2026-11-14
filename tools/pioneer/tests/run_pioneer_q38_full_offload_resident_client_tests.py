#!/usr/bin/env python3
"""Persistent Assistant client IPC tests against the FAKE peer; never hardware."""
import os
import pathlib
import subprocess
import tempfile

here = pathlib.Path(__file__).resolve().parent
project = here.parents[2]
source = project / 'src/apps/assistant/P100AQwen27FullOffload.cpp'
common = [os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
          '-fsanitize=address,undefined', '-DPIONEER_Q38_FULL_OFFLOAD_PROMPT_CLIENT=1']
with tempfile.TemporaryDirectory(prefix='pioneer-full-resident-client-', dir='/private/tmp') as temporary:
    out = pathlib.Path(temporary)
    worker, client = out / 'fake', out / 'client'
    subprocess.run([*common, str(here / 'pioneer_q38_full_offload_ask_fake.cpp'), '-o', str(worker)], check=True)
    define = '-DASSISTANT_QWEN27_FULL_OFFLOAD_WORKER="' + str(worker) + '"'
    subprocess.run([*common, define, str(here / 'pioneer_q38_full_offload_resident_client_test.cpp'),
                    str(source), '-o', str(client)], check=True)
    trace = out / 'trace'
    env = {**os.environ, 'ASK_TEST_TRACE': str(trace), 'UBSAN_OPTIONS': 'halt_on_error=1'}
    result = subprocess.run([str(client)], capture_output=True, text=True, timeout=15, env=env)
    assert result.returncode == 0 and not result.stderr, (result.returncode, result.stdout, result.stderr)
    rows = trace.read_text().splitlines()
    assert rows == ['read 1 1 1', 'read 2 2 2', 'read 2 3 2', 'read 4 4 0'], rows
print('PASS persistent full-card Assistant client;one Load,two requests,one explicit model END;FAKE peer only')
