#!/usr/bin/env python3
"""Posthoc counter-benchmark DATA/rate-format checks and strict first3 reference.

Log consistency is NOT independently established hardware provenance, full-model
numerical correctness, isolated token latency, or Assistant Work acceptance.
No device IO and no runtime CPU inference.
"""
import argparse
import math
import os
import pathlib
import re
import stat
import sys
from compare_pioneer_q38_decode_benchmark_log import POSITIONS,TOTAL_BYTES,compare_first_three
from compare_pioneer_q38_observed_trace import load_cpu,require

HEADER='Q38_FRONTIER_FIXED_2PROMPT_34POSITION_NO_CPU_REFERENCE_NOT_ASSISTANT_WORK_NUMERICAL_VALIDATION_REQUIRED'
OPEN='Q38_FRONTIER_OPEN_AFTER_FULL_SAME_OPEN_GGUF_SHA_AND_COMPLETE_851_LAYOUT'
START='Q38_FRONTIER_START_ACKNOWLEDGED;NOT_COMPLETED_POSITION_OR_TOKEN'
TERMINAL='Q38_FRONTIER_TERMINAL_READY_BEGIN_HOLD_DRAIN_AND_RESTORE;NOT_CLEAN_END_YET'
END='PASS_Q38_FRONTIER uploaded_parts3551 pages4736090 spans20367 records34 final_count34 clean_end1;EXTERNAL_NUMERICAL_COMPARISON_REQUIRED'
UNAVAILABLE='Q38_FRONTIER_RATE_UNAVAILABLE_INSUFFICIENT_POST_PROMPT_WINDOW;CLEAN_END_DOES_NOT_PROVE_RATE'
RATE=re.compile(r'Q38_FRONTIER_OBSERVED_COUNT_RATE_BOUNDS sampled_tokens([0-9]+) first_committed([0-9]+) last_committed([0-9]+) elapsed_min_us([0-9]+) elapsed_max_us([0-9]+) lower_tokens_per_sec([0-9]+\.[0-9]{9}) upper_tokens_per_sec([0-9]+\.[0-9]{9}) observations([0-9]+);POST_PROMPT_COUNT_WINDOW_NOT_ISOLATED_TOKEN_LATENCY_OR_QUALITY_PROOF')

def read_log(path):
    with path.open('rb') as source:
        before=os.fstat(source.fileno())
        require(stat.S_ISREG(before.st_mode) and 0<before.st_size<=512*1024,'log type/length')
        data=source.read(512*1024+1);after=os.fstat(source.fileno())
        identity=lambda s:(s.st_dev,s.st_ino,s.st_size,s.st_mtime_ns,s.st_ctime_ns)
        require(identity(before)==identity(after) and len(data)==before.st_size,'log changed while reading')
    try:lines=data.decode('ascii').splitlines()
    except UnicodeDecodeError:raise ValueError('non-ASCII diagnostic log')
    require(len(lines)>=POSITIONS*8+7 and lines[:2]==[HEADER,OPEN] and lines[-2]==END,'not a complete frontier34 diagnostic')
    at=2;uploaded=0
    while at<len(lines)-2 and lines[at].startswith('Q38_FRONTIER_UPLOADED '):
        match=re.fullmatch(r'Q38_FRONTIER_UPLOADED ([0-9]+)/([0-9]+)',lines[at])
        require(match is not None,'upload format')
        done,total=map(int,match.groups())
        require(total==TOTAL_BYTES and uploaded<done<=total and done%4096==0,'upload order/range')
        uploaded=done;at+=1
    require(uploaded==TOTAL_BYTES and lines[at:at+2]==[START,TERMINAL],'incomplete upload or generation phases')
    at+=2
    require(len(lines)-at==POSITIONS*8+2,'record count')
    records=[[0]*64 for _ in range(POSITIONS)]
    for pos in range(POSITIONS):
        for row in range(8):
            match=re.fullmatch(r'Q38_FRONTIER_RECORD position([0-9]+) word([0-9]{2})((?: [0-9a-f]{8}){8})',lines[at])
            require(match is not None and int(match[1])==pos and int(match[2])==row*8,'record row/order/format')
            records[pos][row*8:row*8+8]=[int(word,16) for word in match[3].split()];at+=1
    for pos,record in enumerate(records):
        input_token=248319 if pos==0 else 17 if pos==1 else records[pos-1][4]
        require(record[:4]==[0x31435451,1,pos,input_token] and record[4]<248320
                and record[5]&0x7f800000!=0x7f800000,'record identity/feedback/finite score')
        require(record[6:12]==[pos+1,(pos+1)&1,int(pos==0),int(pos<POSITIONS-1),64,973],'record flags')
        require(record[12:36]==records[0][12:36] and record[38]==record[42]
                and record[44:47]==[pos+1,48,119] and not any(record[47:]),'session/counter/reserved data')
    rate=None
    if lines[-1]!=UNAVAILABLE:
        match=RATE.fullmatch(lines[-1]);require(match is not None,'rate format')
        tokens,first,last,shortest,longest=map(int,match.groups()[:5])
        lower,upper=map(float,match.groups()[5:7]);observations=int(match[8])
        require(2<=first<=18 and last==POSITIONS and tokens==last-first and tokens>=16,'rate count window')
        require(0<shortest<=longest<=(1<<63)-1 and 2<=observations<=18001,'rate clock/observation range')
        require(math.isfinite(lower) and math.isfinite(upper) and 0<lower<=upper,'rate finite/order')
        require(abs(lower-tokens*1000000.0/longest)<=5.01e-10
                and abs(upper-tokens*1000000.0/shortest)<=5.01e-10,'rate/count/time arithmetic')
        rate={'sampled_tokens':tokens,'first_committed':first,'last_committed':last,
              'elapsed_min_us':shortest,'elapsed_max_us':longest,'lower':lower,'upper':upper,'observations':observations}
    return records,rate

def main(argv=None):
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('cpu',type=pathlib.Path);parser.add_argument('log',type=pathlib.Path)
    args=parser.parse_args(argv)
    require(args.cpu.is_absolute() and args.log.is_absolute(),'absolute paths required')
    require(not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ),'unexpected role override')
    cpu=load_cpu(args.cpu);records,rate=read_log(args.log)
    print('PASS FRONTIER34 record DATA and rate-format consistency;NOT hardware provenance or numerical acceptance')
    print('OBSERVED_COUNT_WINDOW '+str(rate) if rate else 'RATE_UNAVAILABLE')
    return 0 if compare_first_three(cpu,records) else 1
if __name__=='__main__':
    try:sys.exit(main())
    except (ValueError,OSError) as error:
        print(f'FRONTIER34_RECORD_REFUSAL {error}',file=sys.stderr);sys.exit(2)
