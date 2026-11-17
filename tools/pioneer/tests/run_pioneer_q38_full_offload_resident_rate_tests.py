#!/usr/bin/env python3
"""Run the actual two-request IPC benchmark against a synthetic peer."""
import os
from pathlib import Path
import subprocess
import tempfile

here = Path(__file__).resolve().parent
with tempfile.TemporaryDirectory(prefix='q38-resident-rate-test-', dir='/private/tmp') as directory:
    work = Path(directory)
    fake, client = work/'fake', work/'client'
    common = [os.environ.get('CXX','c++'), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
              '-fsanitize=address,undefined']
    subprocess.run([*common, here/'pioneer_q38_full_offload_resident_rate_fake.cpp', '-o', fake], check=True)
    subprocess.run([*common, f'-DPIONEER_FULL_OFFLOAD_SMOKE_WORKER="{fake}"',
                    here.parent/'pioneer_q38_full_offload_resident_rate.cpp', '-o', client], check=True)
    for mode in range(14):
        trace = work/f'trace-{mode}'
        result = subprocess.run([client, '--full-card-resident-rate-32'], capture_output=True, text=True, timeout=15,
                                env={**os.environ, 'RESIDENT_RATE_TRACE': str(trace), 'RESIDENT_RATE_CASE':str(mode),
                                     'UBSAN_OPTIONS':'halt_on_error=1'})
        expected_ids = ([1,2,3,4] if mode in (0,1,9,10,11) else [1] if mode==2 else
                        [1,2,3] if mode in (4,12,13) else [1,2])
        assert [int(r.split()[1]) for r in trace.read_text().splitlines()]==expected_ids, (mode,result)
        assert result.returncode==int(mode>1) and not result.stderr, (mode,result)
        assert ('PASS_RESIDENT_TWO_REQUEST' in result.stdout)==(mode in (0,1)), (mode,result.stdout)
        if mode in (0,1):
            assert 'RESIDENT_REQUEST_CLOSED request1 uploads1 tokens32 ' in result.stdout
            assert 'RESIDENT_REQUEST_CLOSED request2 uploads0 tokens32 ' in result.stdout
            assert result.stdout.count(f'rate_available{int(mode==0)}')==2
        if mode==9:assert 'FAIL_RESIDENT_REFERENCE_ANSWER;MODEL_END_VERIFIED' in result.stdout
        if mode==12:assert 'RESIDENT_WORKER_ERROR Synthetic unknown warm start' in result.stdout
    for args in ([],['--full-card-rate-32'],['--full-card-resident-rate-32','extra']):
        result = subprocess.run([client,*args],capture_output=True,text=True)
        assert result.returncode==2 and not result.stdout and not result.stderr
print('PASS resident two-request IPC: 2 complete, 12 failures, 3 CLI refusals; FAKE only, no card or performance result')
