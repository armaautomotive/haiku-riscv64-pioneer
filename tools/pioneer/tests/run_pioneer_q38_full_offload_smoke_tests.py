#!/usr/bin/env python3
"""Actual smoke-client process/channel logic,FAKE worker only;never Haiku/card."""
import os
import pathlib
import subprocess
import tempfile

here=pathlib.Path(__file__).resolve().parent
with tempfile.TemporaryDirectory(prefix='pioneer-full-offload-smoke-',dir='/private/tmp') as temporary:
    directory=pathlib.Path(temporary);worker=directory/'fake-worker';client=directory/'client'
    common=[os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined']
    subprocess.run([*common,str(here/'pioneer_q38_full_offload_smoke_fake.cpp'),'-o',str(worker)],check=True)
    subprocess.run([*common,'-DPIONEER_FULL_OFFLOAD_SMOKE_WORKER="'+str(worker)+'"',
                    str(here.parent/'pioneer_q38_full_offload_smoke.cpp'),'-o',str(client)],check=True)
    for mode in range(10):
        result=subprocess.run([str(client),'--full-card-hello-four'],stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True,
                              env={**os.environ,'FULL_OFFLOAD_SMOKE_CASE':str(mode),'UBSAN_OPTIONS':'halt_on_error=1'})
        assert result.returncode==(0 if mode==0 else 1),(mode,result.returncode,result.stdout,result.stderr)
        assert not result.stderr,(mode,result.stderr)
        assert ('PASS_FULL_OFFLOAD_HELLO_FOUR' in result.stdout)==(mode==0)
    for args in ([],['--old-mode'],['--full-card-hello-four','extra']):
        result=subprocess.run([str(client),*args],stdout=subprocess.PIPE,stderr=subprocess.PIPE)
        assert result.returncode==2 and not result.stdout and not result.stderr
print('PASS actual full-card smoke client:1 valid/9 malformed-or-failed worker histories/3 invalid CLI refusals;old hybrid Ready/Closed rejected;FAKE worker ONLY,no model/device')
