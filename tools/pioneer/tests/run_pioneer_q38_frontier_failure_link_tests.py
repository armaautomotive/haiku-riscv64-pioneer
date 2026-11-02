#!/usr/bin/env python3
"""Actual QFL1 observer bodies with MOCK authority; no device operation."""
import hashlib
import os
import pathlib
import runpy
import subprocess
import sys
import tempfile

assert len(sys.argv)==1 and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
here=pathlib.Path(__file__).resolve().parent
saved=sys.argv[:]
try:
    sys.argv=[str(here/'run_pioneer_q38_frontier_failure_detail_tests.py')]
    old=runpy.run_path(sys.argv[0],run_name='__main__')
finally:
    sys.argv=saved
sys.stdout.flush()
fixture=(here/'pioneer_q38_frontier_failure_detail_test.cpp').read_text()
assert fixture.count('int main(){')==1
fixture=fixture[:fixture.index('int main(){')]
with tempfile.TemporaryDirectory(prefix='pioneer-q38-frontier-failure-link-',dir='/private/tmp') as name:
    out=pathlib.Path(name);binary=out/'test'
    (out/'actual_frontier_result.inc').write_text(old['record']+'\n')
    (out/'actual_frontier_methods.inc').write_text(old['methods']+'\n')
    (out/'actual_frontier_failure_formatter.inc').write_text(old['formatter']+'\n')
    (out/'unchanged_original_detail_fixture.inc').write_text(fixture)
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror',
                    '-fsanitize=address,undefined','-I'+str(out),'-I'+str(here),
                    str(here/'pioneer_q38_frontier_failure_link_test.cpp'),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True,env={**os.environ,'UBSAN_OPTIONS':'halt_on_error=1'})
print('ACTUAL_QFL1_OBSERVER_PLUS_FAIL_SHA256 '+hashlib.sha256(old['methods'].encode()).hexdigest()+
      ';one explicit pre-error local LINK read,NO following IO after failure;MOCK authority,NOT physical cause or TPS')
