#!/usr/bin/env python3
"""Actual trace client session with synthetic pages and transport; NO DEVICE."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile

assert sys.argv[1:] in ([], ['--quick'], ['--dense-stage'], ['--dense-input'])
dense_stage=sys.argv[1:]==['--dense-stage']
dense_input=sys.argv[1:]==['--dense-input']
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
here=Path(__file__).resolve().parent;root=here.parent
subprocess.run([sys.executable,str(here/'run_pioneer_q38_qgw8_trace_client_role_tests.py')],check=True)
with tempfile.TemporaryDirectory(prefix='qgw8-trace-session-',dir='/private/tmp') as name:
    binary=str(Path(name)/'test')
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O2','-g','-Wall','-Wextra','-Werror',
                    '-Wno-misleading-indentation','-Wno-unused-function','-Wno-unused-variable','-ffp-contract=off',
                    '-fsanitize=address,undefined',*(['-DPIONEER_Q38_QGW8_DENSE_STAGE_CLIENT_132=1'] if dense_stage else []),
                    *(['-DPIONEER_Q38_QGW8_DENSE_INPUT_CLIENT_133=1'] if dense_input else []),
                    str(here/'pioneer_q38_qgw8_trace_session_test.cpp'),
                    str(root/'PioneerModelWeightCache.cpp'),'-o',binary],check=True)
    env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'}
    modes=(['--wrong-input'],['--wrong-tag'],[],['--fail-middle'],['--fail-finish'],['--fail-close'])
    if sys.argv[1:]!=['--quick']:modes+=tuple([mode] for mode in ('--fail-first','--fail-last','--bad-reply'))
    for args in modes:subprocess.run([binary,*args],check=True,env=env)
print('PASS '+('tag133 dense-input' if dense_input else 'tag132 dense-stage' if dense_stage else 'tag131')+' synthetic same-FD upload/hold/drain/128 reads/finish/close lifecycle; NO DEVICE OR TPS')
