#!/usr/bin/env python3
"""Posthoc 34-position DATA consistency and FIRST THREE reference records only.

Never a full-long-generation numerical, native lifetime, physical or TPS proof.
No device IO or runtime CPU model reference.
"""
import argparse
import os
import pathlib
import re
import stat
import sys
from compare_pioneer_q38_observed_trace import load_cpu,require

POSITIONS=34
TOTAL_BYTES=19399024640
HEADER='Q38_DECODE_BENCHMARK_FIXED_2PROMPT_34POSITION_NO_CPU_MODEL_REFERENCE_NOT_ASSISTANT_WORK_OR_STEADY_TOKEN_RATE'
OPEN='Q38_DECODE_BENCHMARK_OPEN_AFTER_FULL_SAME_OPEN_GGUF_SHA_AND_COMPLETE_851_LAYOUT'
START='Q38_DECODE_BENCHMARK_START_ACKNOWLEDGED;NOT_COMPLETED_POSITION_OR_TOKEN'
TERMINAL='Q38_DECODE_BENCHMARK_TERMINAL_READY_BEGIN_HOLD_DRAIN_AND_RESTORE;NOT_CLEAN_END_YET'
END='PASS_Q38_DECODE_BENCHMARK uploaded_parts3551 pages4736090 spans20367 records34 clean_end1;EXTERNAL_NUMERICAL_COMPARISON_REQUIRED_NOT_STEADY_TOKEN_RATE'

def records_from_log(path):
    with path.open('rb') as source:
        before=os.fstat(source.fileno())
        require(stat.S_ISREG(before.st_mode) and 0<before.st_size<=512*1024,'log type/length')
        data=source.read(512*1024+1);after=os.fstat(source.fileno())
        identity=lambda s:(s.st_dev,s.st_ino,s.st_size,s.st_mtime_ns,s.st_ctime_ns)
        require(identity(before)==identity(after) and len(data)==before.st_size,'log changed while reading')
    try:lines=data.decode('ascii').splitlines()
    except UnicodeDecodeError:raise ValueError('non-ASCII diagnostic log')
    require(len(lines)>=POSITIONS*8+6 and lines[:2]==[HEADER,OPEN] and lines[-1]==END,'not a complete34-position diagnostic')
    at=2;uploaded=0
    while at<len(lines)-1 and lines[at].startswith('Q38_DECODE_BENCHMARK_UPLOADED '):
        match=re.fullmatch(r'Q38_DECODE_BENCHMARK_UPLOADED ([0-9]+)/([0-9]+)',lines[at])
        require(match is not None,'upload format')
        done,total=map(int,match.groups())
        require(total==TOTAL_BYTES and uploaded<done<=total and done%4096==0,'upload order/range')
        uploaded=done;at+=1
    require(uploaded==TOTAL_BYTES and lines[at:at+2]==[START,TERMINAL],'incomplete upload or generation phases')
    at+=2
    require(len(lines)-at==POSITIONS*8+1,'record count')
    records=[[0]*64 for _ in range(POSITIONS)]
    for pos in range(POSITIONS):
        for row in range(8):
            match=re.fullmatch(r'Q38_DECODE_BENCHMARK_RECORD position([0-9]+) word([0-9]{2})((?: [0-9a-f]{8}){8})',lines[at])
            require(match is not None and int(match[1])==pos and int(match[2])==row*8,'record row/order/format')
            records[pos][row*8:row*8+8]=[int(word,16) for word in match[3].split()];at+=1
    for pos,record in enumerate(records):
        input_token=248319 if pos==0 else 17 if pos==1 else records[pos-1][4]
        require(record[:4]==[0x31435451,1,pos,input_token] and record[4]<248320
                and record[5]&0x7f800000!=0x7f800000,'record identity/feedback/finite score')
        require(record[6:12]==[pos+1,(pos+1)&1,int(pos==0),int(pos<POSITIONS-1),64,973],'record flags')
        require(record[12:36]==records[0][12:36] and record[38]==record[42]
                and record[44:47]==[pos+1,48,119] and not any(record[47:]),'session/counter/reserved data')
    return records

def compare_first_three(cpu,records):
    require(len(cpu)==3 and len(records)==POSITIONS,'comparison scope')
    exact=True
    for pos,reference in enumerate(cpu):
        h=reference['header'];record=records[pos]
        same_input,same_token,same_score=record[3]==h[3],record[4]==h[4],record[5]==h[6]
        exact &= same_input and same_token and same_score
        print(f'COMPARE_DECODE34_FIRST_THREE position{pos}/sameInput{int(same_input)}/CPUgreedy{h[4]}'
              f'/observedGreedy{record[4]}/selectedMatch{int(same_token)}/maxLogitBitsMatch{int(same_score)}')
    print(f'FIRST_THREE_RECORD_DATA_BITEXACT={int(exact)} NO_TOLERANCE_USED;'
          'positions3..33 have NO external numerical control here;NOT full-hidden/vocabulary/native-lifetime/log-provenance/physical/Work/TPS qualification')
    return bool(exact)

def main(argv=None):
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('cpu',type=pathlib.Path);parser.add_argument('log',type=pathlib.Path)
    args=parser.parse_args(argv)
    require(args.cpu.is_absolute() and args.log.is_absolute(),'absolute paths required')
    require(not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ),'unexpected role override')
    cpu=load_cpu(args.cpu);records=records_from_log(args.log)
    return 0 if compare_first_three(cpu,records) else 1
if __name__=='__main__':
    try:sys.exit(main())
    except (ValueError,OSError) as error:
        print(f'DECODE34_RECORD_REFUSAL {error}',file=sys.stderr);sys.exit(2)
