#!/usr/bin/env python3
"""Returned CPU packet only;old same-FD lifecycle controls FIRST."""
import os
import pathlib
import subprocess
import sys
import tempfile
assert len(sys.argv)==1 and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
here=pathlib.Path(__file__).resolve().parent
subprocess.run([sys.executable,str(here/'pioneer_q38_formatter_graph_test.py')],check=True)
subprocess.run([sys.executable,str(here/'run_pioneer_q38_token_frontier_benchmark_tests.py')],check=True)
with tempfile.TemporaryDirectory(prefix='pioneer-q38-failed-reply-',dir='/private/tmp') as temporary:
    for source in ('pioneer_q38_frontier_failure_report_test.cpp','pioneer_q38_frontier_failure_report_lifecycle_test.cpp'):
        binary=pathlib.Path(temporary)/source.removesuffix('.cpp')
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror',
                        '-ffp-contract=off','-fsanitize=address,undefined',str(here/source),'-o',str(binary)],check=True)
        subprocess.run([str(binary)],check=True,env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
