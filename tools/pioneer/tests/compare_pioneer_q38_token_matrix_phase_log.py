#!/usr/bin/env python3
"""Strict latest-Q4-job DATA plus UNCHANGED phase/frontier/numerical decisions.

No runtime CPU inference or device IO. Per-worker inclusive job cycles are
not model latency,engine utilization,clock frequency or a throughput forecast.
Host labels/returned STOP DATA are not independently authenticated hardware.
"""
import argparse
import contextlib
from fractions import Fraction
import os
import pathlib
import re
import sys
import tempfile
import compare_pioneer_q38_token_phase_profile_log as parent
from compare_pioneer_q38_frontier_failure_report_log import stable_log
from compare_pioneer_q38_observed_trace import load_cpu,require
from compare_pioneer_q38_decode_benchmark_log import compare_first_three

BANNER=b'Q38_MATRIX_PHASE_FIXED34_LATEST_Q4_JOB_HOST_LABEL_NOT_DEVICE_PROVENANCE_OR_WORK\n'
ROW=re.compile(rb'Q38_MATRIX_PHASE_PACKET worker([0-9]{3}) word([0-9]{2})((?: [0-9a-f]{8}){8})\n')
TRAILER=re.compile(rb'Q38_MATRIX_PHASE_AFTER_CLEAN_END workers119 valid([0-9]+) missing([0-9]+) invalid_clock([0-9]+) malformed([0-9]+);LATEST_Q4_JOB_PER_WORKER_INCLUSIVE_BUCKETS_NOT_WHOLE_MODEL_UTILIZATION_OR_FREQUENCY\n')
MISSING,INVALID_CLOCK,INTERVALS,MALFORMED=range(4)
BUCKETS=('FetchWait','DecodeUnpackPack','MatrixAffine')

def classify(raw,worker,stop):
    if not 1<=worker<120 or not 0<stop<0x7fffffff:
        return MALFORMED
    if not any(raw):
        return MISSING
    if (raw[:2]!=[0x324d5051,2] or not 0<raw[2]<stop or raw[3:5]!=[worker,12]
            or raw[5] not in (5120,6144,17408) or not 1<=raw[6]<=16 or raw[7] not in (0,1) or any(raw[14:])):
        return MALFORMED
    total=0
    for at in (8,10,12):
        value=raw[at] | raw[at+1]<<32
        if (value==0 if raw[7] else value!=0):
            return MALFORMED
        total+=value
        if total>=(1<<64):
            return MALFORMED
    return INTERVALS if raw[7] else INVALID_CLOCK

def decode_packet(words,worker):
    require(len(words)==48 and all(isinstance(w,int) and 0<=w<(1<<32) for w in words),'matrix packet length/domain')
    require(1<=worker<120 and words[:8]==[1,1,worker,0,0,0,0,0],'matrix exact request/worker')
    require(words[8:15]==[1,1,0,1,1,0,worker] and 0<words[15]<0x7fffffff,'matrix safe ready reply/STOP bound')
    raw=words[32:];state=classify(raw,worker,words[15])
    require(words[16:32]==[1,1,4,worker,63,0,1,int(state==INTERVALS),state,raw[2],raw[5],raw[6],0,0,0,0],
            'matrix detail/raw/valid/metric state mismatch')
    result={'worker':worker,'state':state,'valid':state==INTERVALS,'packet_words':tuple(words),'buckets':[]}
    if state==INTERVALS:
        cycles=[raw[i] | raw[i+1]<<32 for i in (8,10,12)];total=sum(cycles)
        result.update(inputs=raw[5],groups=raw[6],nonce=raw[2],stop_nonce=words[15],total_cycles=total)
        result['buckets']=[(name,value,Fraction(value,total)) for name,value in zip(BUCKETS,cycles)]
    return result

@contextlib.contextmanager
def snapshot(stdout,stderr):
    data=stable_log(stdout,512*1024)
    require(stable_log(stderr,0)==b'' and not os.path.samefile(stdout,stderr),'matrix stderr nonempty/not distinct')
    require(data.startswith(BANNER) and data.count(BANNER)==1,'exact unique matrix host label')
    remainder=data[len(BANNER):]
    require(remainder.startswith(parent.BANNER),'unchanged phase envelope required')
    lines=remainder.splitlines(keepends=True);count=119*6
    require(len(lines)>count+1,'missing matrix packet tail')
    trailer=TRAILER.fullmatch(lines[-1]);require(trailer is not None,'matrix trailer')
    samples=[];states=[0]*4;rows=lines[-count-1:-1]
    for worker in range(1,120):
        words=[]
        for row in range(6):
            match=ROW.fullmatch(rows[(worker-1)*6+row])
            require(match is not None and int(match[1])==worker and int(match[2])==row*8,'matrix packet worker/order/row/format')
            words.extend(int(w,16) for w in match[3].split())
        sample=decode_packet(words,worker);samples.append(sample);states[sample['state']]+=1
    declared=[int(trailer[i]) for i in range(1,5)]
    require(declared==[states[INTERVALS],states[MISSING],states[INVALID_CLOCK],states[MALFORMED]],'matrix trailer counts')
    with tempfile.TemporaryDirectory(prefix='pioneer-matrix-normalized-') as directory:
        path=pathlib.Path(directory)/'MATRIX-ENVELOPE-REMOVED-NOT-RAW-HARDWARE.log'
        path.write_bytes(b''.join(lines[:-count-1]))
        yield path,{'samples':samples,'states':tuple(states),'all_valid':states[INTERVALS]==119}

def read_log(stdout,stderr):
    with snapshot(stdout,stderr) as (path,matrix):
        records,rate,phase=parent.read_log(path,stderr)
        return records,rate,phase,matrix

def main(argv=None):
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('cpu',type=pathlib.Path);parser.add_argument('stdout',type=pathlib.Path);parser.add_argument('stderr',type=pathlib.Path)
    args=parser.parse_args(argv)
    require(args.cpu.is_absolute(),'absolute original CPU trace directory required')
    require(not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ),'unexpected role override')
    cpu=load_cpu(args.cpu);records,rate,phase,matrix=read_log(args.stdout,args.stderr)
    print('PASS MATRIX119 fixed packets/independent STOP-bound grammar/UNCHANGED phase+frontier34 checks;NOT hardware provenance or Work')
    print('OBSERVED_COUNT_WINDOW '+str(rate) if rate else 'RATE_UNAVAILABLE')
    print('Q4_JOB_PROFILE_SCOPE latest individual completed Q4 job per worker/inclusive intervals/setup+gaps+publication excluded;'
          'NOT full-model latency,total parallel time,pure engine utilization,clock frequency or TPS forecast')
    for sample in matrix['samples']:
        print(f"Q4_JOB_SAMPLE worker{sample['worker']:03d} metric_state{sample['state']} valid{int(sample['valid'])}")
        if sample['valid']:
            print(f"Q4_JOB_SHAPE K{sample['inputs']} groups{sample['groups']} nonce{sample['nonce']} stop_bound{sample['stop_nonce']} total_inclusive_cycles{sample['total_cycles']}")
            for name,cycles,fraction in sample['buckets']:print(f'Q4_JOB_BUCKET {name} cycles{cycles} fraction{fraction}')
    if phase['valid']:
        for name,count,cycles,fraction in phase['buckets']:print(f'DEVICE_CYCLE_BUCKET {name} segments{count} cycles{cycles} fraction{fraction}')
    else:print('INVALID_COARSE_PHASE_DATA_CLEAN_END_RETAINED')
    # Preserve the exact original three-position CPU pins and decision. A
    # correct packet/profile never overrides a numerical mismatch.
    if not compare_first_three(cpu,records):return 1
    return 0 if matrix['all_valid'] and phase['valid'] else 2

if __name__=='__main__':
    try:sys.exit(main())
    except (ValueError,OSError) as error:
        print('MATRIX_PHASE_RECORD_REFUSAL '+str(error),file=sys.stderr);sys.exit(2)
