#!/usr/bin/env python3
"""Compare actual old/opt-in observer methods using MOCK authority only."""
import hashlib
import os
import pathlib
import subprocess
import sys
import tempfile

assert len(sys.argv)==1 and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
here=pathlib.Path(__file__).resolve().parent
subprocess.run([sys.executable,str(here/'run_pioneer_q38_token_frontier_method_tests.py')],check=True)
source=(here.parents[2]/'src/add-ons/kernel/drivers/misc/PioneerQ38TokenNativeOwner.h').read_text()
def extract(signature,text=source):
    assert text.count(signature)==1
    begin=text.index(signature);brace=text.index('{',begin);depth=1;end=brace+1
    while depth:
        depth+=(text[end]=='{')-(text[end]=='}');end+=1
    return text[begin:end]
record=extract('struct PioneerQ38TokenFrontierResult {')+';'
methods='\n'.join(extract(signature) for signature in (
    'template<class IO> bool Fail(IO& io,PioneerQ38TokenFrontierResult& r)',
    'template<class IO> __attribute__((noinline)) bool ObserveCommittedFrontier('))
formatter=extract('static __attribute__((noinline)) bool PioneerQ38WriteFrontierFailedReply(',
                  (here.parent/'PioneerQ38FrontierFailureReport.h').read_text())
with tempfile.TemporaryDirectory(prefix='pioneer-q38-frontier-failure-detail-',dir='/private/tmp') as name:
    out=pathlib.Path(name);binary=out/'test'
    (out/'actual_frontier_result.inc').write_text(record+'\n')
    (out/'actual_frontier_methods.inc').write_text(methods+'\n')
    (out/'actual_frontier_failure_formatter.inc').write_text(formatter+'\n')
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror',
                    '-fsanitize=address,undefined','-I'+str(out),
                    str(here/'pioneer_q38_frontier_failure_detail_test.cpp'),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True,env={**os.environ,'UBSAN_OPTIONS':'halt_on_error=1'})
print('ACTUAL_OBSERVER_PLUS_FAIL_SHA256 '+hashlib.sha256(methods.encode()).hexdigest()+
      ';private opt-in failure DATA ONLY,source/boot/quarantine MOCK;NO physical binding or retired010c retry grant')
