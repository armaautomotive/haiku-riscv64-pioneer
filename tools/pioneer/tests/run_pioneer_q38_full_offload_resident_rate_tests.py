#!/usr/bin/env python3
"""Run the actual two-request IPC benchmark against a synthetic peer."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile

here = Path(__file__).resolve().parent
sys.path.insert(0,str(here.parent))
from inspect_qwen38_vector_resident_rate import InvalidLog, inspect


def inspect_fake(text, measured):
    parsed = inspect(text)
    first, second = parsed['requests']
    assert first['uploaded_bytes']==parsed['total_uploaded_bytes']==19399024640
    assert first['weight_upload_rows']==1 and second['weight_upload_rows']==second['uploaded_bytes']==0
    assert all((r['observed_post_prompt_count_rate_bounds'] is not None)==measured for r in (first,second))
    assert parsed['model_end_marker'] and not parsed['execution_or_cleanup_authority_from_log']
    if not measured:
        return
    upload = text.splitlines()[2]
    changes=(
        ('FULL_OFFLOAD_PROGRESS id3 value4294967295 Qwen27B',upload.replace('id2','id3')+'\n'
         'FULL_OFFLOAD_PROGRESS id3 value4294967295 Qwen27B'),
        ('request2 uploads0','request2 uploads1'),
        ('request2 uploads0','request1 uploads0'),
        ('request2 uploads0 tokens32','request2 uploads0 tokens30'),
        ('id3 value4294967295 Full-card count committed31','id2 value4294967295 Full-card count committed31'),
        ('first_begin_us10001000','first_begin_us1000'),
        ('model_end1 worker_exit0','model_end0 worker_exit0'),
        ('RESIDENT_ANSWER request2 bytes32','RESIDENT_ANSWER request2 bytes31'),
        ('rate_available1 P100A','rate_available0 P100A'),
        ('AFTER_CLEAN_REQUEST_CLOSE,model reservation','AFTER_CLEAN_END,model reservation'),
    )
    for old,new in changes:
        assert old in text
        try:
            inspect(text.replace(old,new,1))
        except InvalidLog:
            pass
        else:
            raise AssertionError(old)
    print('PASS independent resident reader rejected 10 transcript corruptions, including a second upload')


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
            inspect_fake(result.stdout,mode==0)
        if mode==9:assert 'FAIL_RESIDENT_REFERENCE_ANSWER;MODEL_END_VERIFIED' in result.stdout
        if mode==12:assert 'RESIDENT_WORKER_ERROR Synthetic unknown warm start' in result.stdout
    for args in ([],['--full-card-rate-32'],['--full-card-resident-rate-32','extra']):
        result = subprocess.run([client,*args],capture_output=True,text=True)
        assert result.returncode==2 and not result.stdout and not result.stderr
print('PASS resident two-request IPC: 2 complete, 12 failures, 3 CLI refusals; FAKE only, no card or performance result')
