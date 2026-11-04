#!/usr/bin/env python3
"""Actual longer client and rate DATA tests;FAKE worker only,never hardware."""
import os
import pathlib
import subprocess
import tempfile

here=pathlib.Path(__file__).resolve().parent
with tempfile.TemporaryDirectory(prefix='pioneer-full-rate-tests-',dir='/private/tmp') as temporary:
    out=pathlib.Path(temporary);worker=out/'fake';client=out/'client';data=out/'data';samples=out/'samples'
    common=[os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined']
    for source,binary in ((here/'pioneer_q38_full_offload_rate_fake.cpp',worker),
                          (here/'pioneer_q38_full_offload_rate_data_test.cpp',data)):
        subprocess.run([*common,str(source),'-o',str(binary)],check=True)
    subprocess.run([str(data)],check=True)
    subprocess.run([*common,'-DB_DEVICE_OP_CODES_END=1000',str(here/'pioneer_q38_token_rate_samples_test.cpp'),'-o',str(samples)],check=True)
    subprocess.run([str(samples)],check=True)
    subprocess.run([*common,'-DPIONEER_FULL_OFFLOAD_SMOKE_WORKER="'+str(worker)+'"',
                    str(here.parent/'pioneer_q38_full_offload_rate_smoke.cpp'),'-o',str(client)],check=True)
    for mode in range(13):
        result=subprocess.run([str(client),'--full-card-rate-32'],capture_output=True,text=True,timeout=15,
                              env={**os.environ,'FULL_OFFLOAD_RATE_CASE':str(mode),'UBSAN_OPTIONS':'halt_on_error=1'})
        assert result.returncode==(0 if mode in (0,1) else 1),(mode,result.returncode,result.stdout,result.stderr)
        assert not result.stderr,(mode,result.stderr)
        assert ('PASS_FULL_OFFLOAD_RATE_JOB' in result.stdout)==(mode in (0,1))
        if mode in (0,1):assert 'rate_available'+str(int(mode==0)) in result.stdout
    for args in ([],['--full-card-hello-four'],['--full-card-rate-32','extra']):
        result=subprocess.run([str(client),*args],capture_output=True)
        assert result.returncode==2 and not result.stdout and not result.stderr
print('PASS actual full-card32 request client:2 valid available/unavailable jobs,11 malformed/failing histories,3 CLI refusals;raw DATA arithmetic and real process/channel lifecycle;FAKE worker ONLY,no device/model or measured TPS')
