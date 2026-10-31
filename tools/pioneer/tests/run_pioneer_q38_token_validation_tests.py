#!/usr/bin/env python3
"""Offline tag74 grammar/role/routing checks, never module or device use."""
import os
import pathlib
import subprocess
import sys
import tempfile

assert len(sys.argv) == 1 and not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
here = pathlib.Path(__file__).resolve().parent
with tempfile.TemporaryDirectory(prefix='pioneer-token-validation-', dir='/private/tmp') as temporary:
    binary = pathlib.Path(temporary)/'packet'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=address,undefined', str(here/'pioneer_q38_token_validation_packet_test.cpp'),
                    '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True, env={**os.environ, 'UBSAN_OPTIONS':'halt_on_error=1'})
for script in ('run_pioneer_q38_token_validation_role_tests.py', 'run_pioneer_q38_token_validation_routing_tests.py',
               'run_pioneer_q38_token_control_packet_tests.py', 'run_pioneer_q38_token_native_role_tests.py',
               'run_pioneer_q38_recurrent_prefix_validation_routing_tests.py'):
    subprocess.run([sys.executable, str(here/script)], check=True)
print('PASS tag74 OFFLINE packet+role+routing and old packet+NODRV role+prefix routing controls;NO module load,lifecycle,original-model,physical or TPS proof')
