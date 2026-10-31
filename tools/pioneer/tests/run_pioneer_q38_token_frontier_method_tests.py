#!/usr/bin/env python3
"""Actual production observer/Fail methods; all authority explicitly MOCK."""
import hashlib
import os
import pathlib
import subprocess
import tempfile

assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
here=pathlib.Path(__file__).resolve().parent
source=(here.parents[2]/'src/add-ons/kernel/drivers/misc/PioneerQ38TokenNativeOwner.h').read_text()
def extract(signature):
    assert source.count(signature)==1
    begin=source.index(signature);brace=source.index('{',begin);depth=1;end=brace+1
    while depth:
        depth+=(source[end]=='{')-(source[end]=='}');end+=1
    return source[begin:end]
record=extract('struct PioneerQ38TokenFrontierResult {')+';'
methods='\n'.join(extract(signature) for signature in (
    'template<class IO> bool Fail(IO& io,PioneerQ38TokenFrontierResult& r)',
    'template<class IO> __attribute__((noinline)) bool ObserveCommittedFrontier('))
with tempfile.TemporaryDirectory(prefix='pioneer-q38-frontier-method-',dir='/private/tmp') as name:
    out=pathlib.Path(name);binary=out/'test'
    (out/'actual_frontier_result.inc').write_text(record+'\n')
    (out/'actual_frontier_methods.inc').write_text(methods+'\n')
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror',
                    '-fsanitize=address,undefined','-I'+str(out),str(here/'pioneer_q38_token_frontier_method_test.cpp'),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True,env={**os.environ,'UBSAN_OPTIONS':'halt_on_error=1'})
print('ACTUAL_OBSERVER_PLUS_FAIL_SHA256 '+hashlib.sha256(methods.encode()).hexdigest()+
      ';extracted methods ONLY,scope/source/boot-seal/quarantine are MOCK,not genuine native runtime or hardware qualification')
