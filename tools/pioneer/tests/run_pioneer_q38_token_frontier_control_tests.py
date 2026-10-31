#!/usr/bin/env python3
"""Actual adapter/packet; OS,cookie,source,observer,retention explicitly MOCK."""
import hashlib
import os
import pathlib
import subprocess
import tempfile

assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
here=pathlib.Path(__file__).resolve().parent;kernel=here.parents[2]/'src/add-ons/kernel/drivers/misc'
def extract(source,signature):
    assert source.count(signature)==1
    begin=source.index(signature);brace=source.index('{',begin);depth=1;end=brace+1
    while depth:
        depth+=(source[end]=='{')-(source[end]=='}');end+=1
    return source[begin:end]
owner=(kernel/'PioneerQ38TokenNativeOwner.h').read_text()
record=extract(owner,'struct PioneerQ38TokenFrontierResult {')+';'
source=(kernel/'PioneerQ38TokenFrontierControl.inc').read_text()
adapter=extract(source,'[[maybe_unused]] static __attribute__((noinline)) status_t\nQ38TokenFrontierControl(')
with tempfile.TemporaryDirectory(prefix='pioneer-q38-frontier-control-',dir='/private/tmp') as name:
    out=pathlib.Path(name);binary=out/'test'
    (out/'actual_frontier_result.inc').write_text(record+'\n');(out/'actual_frontier_control.inc').write_text(adapter+'\n')
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror',
                    '-fsanitize=address,undefined','-I'+str(out),str(here/'pioneer_q38_token_frontier_control_test.cpp'),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True,env={**os.environ,'UBSAN_OPTIONS':'halt_on_error=1'})
print('ACTUAL_PRIVATE_FRONTIER_ADAPTER_SHA256 '+hashlib.sha256(adapter.encode()).hexdigest()+
      ';no public ProbeControl route/device access;MOCK FD/source/observer/OS authority,not full native runtime/physical/timing/Work qualification')
