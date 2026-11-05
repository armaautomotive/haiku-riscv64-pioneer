#!/usr/bin/env python3
"""Read a COMPLETE,already-closed fixed Hello/four-token client transcript.

DATA consistency only. A log/parsed result is never execution,ownership,
cleanup or driver-exchange authority. The caller must first observe the
actual parent terminal status and separately audit installed/restored state.
No CPU model reference,no device IO,no token-rate extrapolation.
"""
import argparse
from decimal import Decimal, ROUND_CEILING, ROUND_FLOOR
import json
import os
import pathlib
import re

TOTAL=19399024640
PREPARED_TOTAL=20470759424
MIB=1048576
AUTH='FULL_OFFLOAD_PROGRESS id1 value4294967295 Authenticating Qwen27B; CPU will load vocabulary only,not model weights or an inference context.'
READY='FULL_OFFLOAD_VOCABULARY_READY_NO_CPU_CONTEXT;GPU_UPLOAD_NOT_YET_STARTED'
RUNNING='FULL_OFFLOAD_PROGRESS id2 value4294967295 Qwen27B running entirely on P100A: embeddings,matrices,attention,KV,recurrent state,norms and greedy selection. CPU inference OFF.'
ANSWER='FULL_OFFLOAD_PUBLIC_ANSWER bytes14 text=Hello! How can'
FINAL='PASS_FULL_OFFLOAD_HELLO_FOUR clean_worker_exit1 expected_public_text1;CPU_INFERENCE_CONTEXTS0;NOT_STEADY_TOKEN_RATE_OR_BROAD_MODEL_QUALITY'
NUMBER=r'(?:0|[1-9][0-9]*)'
UPLOAD=re.compile(r'FULL_OFFLOAD_PROGRESS id2 value('+NUMBER+r') Full-card Qwen27B upload ('+NUMBER+r'\.[0-9])/18500\.4 MiB\. No CPU inference\.')
DONE=re.compile(r'FULL_OFFLOAD_DONE P100A full-offload candidate: 4 output tokens; prompt\+generation ('+NUMBER+r'\.[0-9]{3})s,total ('+NUMBER+r'\.[0-9]{3})s\. All inference on card; CPU vocabulary/text only\. Clean END; weights released\. Not a warm-token speed measurement\.')
class InvalidLog(ValueError):
    pass
def require(condition,message):
    if not condition:raise InvalidLog(message)
def upload_prefix(lines,cursor,total=TOTAL):
    require(type(total) is int and total in (TOTAL,PREPARED_TOTAL),'known exact source upload layout required')
    displayed=format(total/MIB,'.1f')
    pattern=UPLOAD if total==TOTAL else re.compile(r'FULL_OFFLOAD_PROGRESS id2 value('+NUMBER+r') Full-card Qwen27B upload ('+NUMBER+r'\.[0-9])/'+re.escape(displayed)+r' MiB\. No CPU inference\.')
    events=0;previous=0
    while cursor<len(lines) and lines[cursor]!=RUNNING:
        match=pattern.fullmatch(lines[cursor]);require(match is not None,'unexpected,failed or out-of-order upload line')
        value=int(match[1]);shown=Decimal(match[2]);require(0<=value<=1000 and shown<=Decimal(displayed),'upload bounds')
        # Display rounds to0.1MiB;percent is floor(realBytes*1000/TOTAL).
        # Require at least one increasing,4096-aligned original byte count
        # that satisfies BOTH fields,not an invented exact count from text.
        low=max(0,int(((shown-Decimal('.05'))*MIB).to_integral_value(rounding=ROUND_CEILING)))
        high=min(total,int(((shown+Decimal('.05'))*MIB).to_integral_value(rounding=ROUND_FLOOR)))
        low=max(low,(value*total+999)//1000,previous+4096)
        high=min(high,((value+1)*total+999)//1000-1)
        low=(low+4095)//4096*4096
        require(low<=high,'upload fields cannot describe increasing aligned bytes')
        previous=low;events+=1;cursor+=1
    require(events>0 and previous==total,'complete upload required,not a progress prefix')
    return cursor,events
def parse_complete(text,stderr=''):
    require(isinstance(stderr,str) and not stderr,'nonempty/unknown stderr is not an accepted successful transcript')
    require(isinstance(text,str) and len(text)<=16*1024*1024 and text.isascii()
            and text.endswith('\n') and '\r' not in text and '\0' not in text,'whole ASCII/LF transcript required')
    lines=text[:-1].split('\n')
    require(len(lines)>=7 and lines[:2]==[AUTH,READY],'exact authentication/vocabulary prefix required')
    cursor,events=upload_prefix(lines,2)
    require(len(lines)-cursor==4 and lines[cursor]==RUNNING,'complete execution/done/answer/final suffix required')
    match=DONE.fullmatch(lines[cursor+1]);require(match is not None,'exact four-output completion required')
    require(lines[cursor+2]==ANSWER and lines[cursor+3]==FINAL,'expected public answer and full-worker clean close required')
    compute,total=Decimal(match[1]),Decimal(match[2])
    timing_valid=compute>0 and total>=compute
    return {'scope':'one fixed public Hello/four-output full-card candidate',
            'output_tokens':4,'answer':'Hello! How can','upload_events':events,
            'uploaded_bytes':TOTAL,'reported_post_start_ack_seconds':str(compute),
            'reported_generate_total_seconds':str(total),'timing_fields_valid':timing_valid,
            'warm_token_rate':None,'broad_model_quality':False,
            'execution_or_cleanup_authority_from_log':False}
def stable_text(path):
    with pathlib.Path(path).open('rb') as file:
        before=os.fstat(file.fileno())
        data=file.read(16*1024*1024+1);after=os.fstat(file.fileno())
    identity=lambda s:(s.st_dev,s.st_ino,s.st_size,s.st_mtime_ns,s.st_ctime_ns)
    require(identity(before)==identity(after) and len(data)==after.st_size and len(data)<=16*1024*1024,'changed/oversized input')
    try:return data.decode('ascii')
    except UnicodeError as error:raise InvalidLog('non-ASCII input') from error
def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('closed_stdout',type=pathlib.Path)
    parser.add_argument('closed_stderr',type=pathlib.Path)
    args=parser.parse_args()
    try:result=parse_complete(stable_text(args.closed_stdout),stable_text(args.closed_stderr))
    except (OSError,InvalidLog) as error:raise SystemExit('REFUSE_FULL_CARD_LOG '+str(error))
    print(json.dumps(result,sort_keys=True))
    print('PASS_COMPLETE_FULL_CARD_TRANSCRIPT_DATA_ONLY;NOT_STEADY_TOKEN_RATE_BROAD_QUALITY_OR_NATIVE_CLEANUP_AUTHORITY')
if __name__=='__main__':main()
