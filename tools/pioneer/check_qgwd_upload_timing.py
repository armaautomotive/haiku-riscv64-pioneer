#!/usr/bin/env python3
"""Validate tag142 timing transcript shape; no quality or device claim."""
import re
import subprocess
import sys
from pathlib import Path

def check(data):
    rows=data.splitlines()
    pattern=(r'HYBRID_UPLOAD_TIMING valid=1 read_calls=([0-9]+) read_bytes=([0-9]+) read_ns=([0-9]+) '
             r'span_calls=([0-9]+) span_bytes=([0-9]+) span_ioctl_ns=([0-9]+);HOST_WALL_TIME_NOT_RAW_PCIE')
    found=[(i,re.fullmatch(pattern,row)) for i,row in enumerate(rows) if row.startswith('HYBRID_UPLOAD_TIMING')]
    if len(found)!=1 or found[0][1] is None:raise ValueError('missing or malformed timing line')
    index,match=found[0];read_calls,read_bytes,read_ns,span_calls,span_bytes,span_ns=map(int,match.groups())
    if index==0 or index+1>=len(rows) or rows[index-1]!='HYBRID_START_REQUEST diagnostic only; no completion implied':
        raise ValueError('timing line order')
    started=re.fullmatch(r'HYBRID_STARTED upload_and_start_seconds=([0-9]+)\.([0-9]{3})',rows[index+1])
    if not started or not (read_calls>0 and span_calls==21907 and read_bytes==span_bytes==21094223872):
        raise ValueError('timing counts')
    total_ns=(int(started[1])*1000+int(started[2]))*1000000
    if not total_ns or read_ns+span_ns>total_ns+1000000:raise ValueError('timing exceeds total')
    gate=Path(__file__).resolve().parent/'pioneer_q38_qgwd_two_candidate_output.awk'
    result=subprocess.run(['awk','-f',str(gate)],input='\n'.join(rows[:index]+rows[index+1:])+'\n',
                          text=True,capture_output=True,check=False)
    if result.returncode:raise ValueError('original tag142 shape failed')
    return f'QGWD_UPLOAD_TIMING_SHAPE_CLEAN read_ns={read_ns} span_ioctl_ns={span_ns};NOT_RAW_PCIE_OR_QUALITY'

if __name__=='__main__':
    try:print(check(sys.stdin.read()))
    except ValueError as error:
        print('QGWD_UPLOAD_TIMING_REFUSED '+str(error),file=sys.stderr);sys.exit(1)
