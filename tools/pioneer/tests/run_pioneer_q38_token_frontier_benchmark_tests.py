#!/usr/bin/env python3
"""Exact USER lifecycle with synthetic source/OS/transport,NO device IO."""
import os
import pathlib
import subprocess
import tempfile
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
here=pathlib.Path(__file__).resolve().parent
with tempfile.TemporaryDirectory(prefix='pioneer-q38-frontier-benchmark-',dir='/private/tmp') as name:
    binary=pathlib.Path(name)/'test'
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror',
                    '-ffp-contract=off','-fsanitize=address,undefined',str(here/'pioneer_q38_token_frontier_benchmark_test.cpp'),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True,env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
print('PASS separate frontier same-FD counter+generation USER fixture;SYNTHETIC transport only,NOT native/model/physical/timing/Work readiness')
