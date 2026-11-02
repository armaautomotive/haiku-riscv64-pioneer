#!/usr/bin/env python3
"""SYNTHETIC119-worker envelopes;not device timing or numerical evidence."""
import contextlib
from fractions import Fraction
import io
import os
import pathlib
import runpy
import subprocess
import sys
import tempfile
import compare_pioneer_q38_token_matrix_phase_log as candidate

assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
here=pathlib.Path(__file__).resolve().parent
old=runpy.run_path(str(here/'pioneer_q38_token_phase_profile_comparator_test.py'))
accepted=refused=0
def packet(worker,state=2):
    raw=[0]*16
    if state:
        raw[:8]=[0x324d5051,2,17,worker,12,17408,3,int(state==2)]
        if state==2:raw[8:14]=[7,3,11,5,13,7]
        if state==3:raw[0]^=1
    detail=[1,1,4,worker,63,0,1,int(state==2),state,raw[2],raw[5],raw[6],0,0,0,0]
    return [1,1,worker,0,0,0,0,0]+[1,1,0,1,1,0,worker,19]+detail+raw

def envelope(packets=None,states=None,phase=None):
    packets=[packet(w) for w in range(1,120)] if packets is None else packets
    states=[2]*119 if states is None else states
    phase=old['envelope']() if phase is None else phase
    rows=[]
    for worker,words in enumerate(packets,1):
        for at in range(0,48,8):
            rows.append(f'Q38_MATRIX_PHASE_PACKET worker{worker:03d} word{at:02d}'.encode()
                        +b''.join(f' {word:08x}'.encode() for word in words[at:at+8])+b'\n')
    trailer=(f'Q38_MATRIX_PHASE_AFTER_CLEAN_END workers119 valid{states.count(2)} missing{states.count(0)} '
             f'invalid_clock{states.count(1)} malformed{states.count(3)};'
             'LATEST_Q4_JOB_PER_WORKER_INCLUSIVE_BUCKETS_NOT_WHOLE_MODEL_UTILIZATION_OR_FREQUENCY\n').encode()
    return candidate.BANNER+phase+b''.join(rows)+trailer

with tempfile.TemporaryDirectory(prefix='pioneer-matrix-SYNTHETIC-',dir='/private/tmp') as directory:
    root=pathlib.Path(directory);stdout=root/'SYNTHETIC.stdout';stderr=root/'SYNTHETIC.stderr'
    def check(data,okay,states=(0,0,119,0),error=b'',coarse=True):
        global accepted,refused
        stdout.write_bytes(data);stderr.write_bytes(error);before=stdout.read_bytes(),stderr.read_bytes()
        try:records,rate,phase,matrix=candidate.read_log(stdout,stderr)
        except (ValueError,OSError):
            assert not okay;refused+=1
        else:
            assert okay and records==old['records']() and phase['valid']==coarse
            assert matrix['states']==states and matrix['all_valid']==(states[2]==119) and len(matrix['samples'])==119
            for sample in matrix['samples']:
                if sample['valid']:
                    assert sum(row[2] for row in sample['buckets'])==Fraction(1)
                    assert sum(row[1] for row in sample['buckets'])==sample['total_cycles']
                else:assert not sample['buckets'] and 'total_cycles' not in sample
            accepted+=1
        assert before==(stdout.read_bytes(),stderr.read_bytes()),'RAW_LOG_REWRITTEN'
    original=envelope();check(original,True)
    for state in (0,1,3):
        values=[packet(w,state if w==60 else 2) for w in range(1,120)]
        states=[2]*119;states[59]=state;counts=[0]*4;counts[state]=1;counts[2]=118
        check(envelope(values,states),True,tuple(counts))
    check(envelope(phase=old['envelope'](old['bad_metric'])),True,coarse=False)
    # Cross-check independent Python DATA grammar against the actual C++
    # packet ReplyShape,including syntactically plausible changed cycles or
    # STOP bounds. Neither parser can authenticate their provenance.
    cases=[packet(w,state) for w in (1,60,119) for state in range(4)]
    for word in range(48):
        for bit in range(32):
            p=packet(1);p[word]^=1<<bit;cases.append(p)
    # Worker identity is part of the external ordered row as well as packet.
    # For the mutation corpus it is ALWAYS1,not the possibly damaged request.
    expected=[]
    for index,p in enumerate(cases):
        try:candidate.decode_packet(p,p[2] if index<12 else 1)
        except ValueError:expected.append(0)
        else:expected.append(1)
    binary=root/'packet-data'
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror',
                    '-fsanitize=address,undefined',str(here/'pioneer_q38_token_matrix_phase_packet_data.cpp'),'-o',str(binary)],check=True)
    result=subprocess.run([str(binary)],input=''.join(' '.join(map(str,p))+'\n' for p in cases),
                          text=True,capture_output=True,check=True)
    assert not result.stderr and list(map(int,result.stdout.splitlines()))==expected
    accepted+=len(cases)
    # Pure C++ sees packet worker only. Corrupting request.worker alone still
    # fails echo consistency,so the externally bound corpus agrees exactly.
    lines=original.splitlines(keepends=True);first=len(lines)-119*6-1
    for row in range(119*6):
        at=first+row
        check(b''.join(lines[:at]+lines[at+1:]),False)
        check(b''.join(lines[:at]+[lines[at]]+lines[at:]),False)
    for at in range(len(candidate.BANNER)):
        changed=bytearray(original);changed[at]^=1;check(bytes(changed),False)
    for data in (b'',candidate.BANNER,old['envelope'](),candidate.BANNER+original,original+b'EXTRA\n',
                 original[:-1],original.replace(b'\n',b'\r\n'),original.replace(b'worker001',b'worker000'),
                 original.replace(b'workers119 valid119',b'workers119 valid118'),
                 original.replace(b'clean_end1',b'clean_end0'),original.replace(b'word40',b'word32')):
        check(data,False)
    for error in (b' ',b'\n',b'FAILED_IOCTL\n',bytes(range(256))):check(original,False,error=error)
    stdout.write_bytes(original);stderr.write_bytes(b'')
    for paths in ((pathlib.Path('relative'),stderr),(stdout,pathlib.Path('relative')),(root/'missing',stderr),
                  (stdout,root/'missing'),(stdout,stdout)):
        try:candidate.read_log(*paths)
        except (ValueError,OSError):refused+=1
        else:raise AssertionError('invalid file identity accepted')
    link=root/'linked'
    for target in (stdout,stderr):
        link.symlink_to(target)
        try:candidate.read_log(link if target==stdout else stdout,link if target==stderr else stderr)
        except ValueError:refused+=1
        else:raise AssertionError('symlink accepted')
        link.unlink() # exact temporary TEST link only
    # Real CLI decisions with explicitly synthetic loader values. Production
    # CPU pins,compare_first_three,tolerances and numeric decisions unchanged.
    rows=old['records']();cpu=[{'header':[0,0,pos,row[3],row[4],0,row[5]]} for pos,row in enumerate(rows[:3])]
    loader=candidate.load_cpu;candidate.load_cpu=lambda path:cpu
    try:
        for fine in (True,False):
            for coarse in (True,False):
                for mismatch in (False,True):
                    records=old['records']()
                    if mismatch:records[0][5]^=1
                    phase=old['envelope'](None if coarse else old['bad_metric'],old['fixture'](records).encode())
                    values=[packet(w,2 if fine or w!=60 else 0) for w in range(1,120)]
                    states=[2]*119
                    if not fine:states[59]=0
                    stdout.write_bytes(envelope(values,states,phase));stderr.write_bytes(b'')
                    with contextlib.redirect_stdout(io.StringIO()):status=candidate.main([str(root),str(stdout),str(stderr)])
                    assert status==(1 if mismatch else 0 if fine and coarse else 2);accepted+=1
    finally:candidate.load_cpu=loader
    for key in ('PIONEER_Q38_WORK_INFERENCE','PIONEER_Q38_CPU_VALIDATION','TT_PIONEER_Q38_WORK_MODE'):
        for value in ('0','1'):
            result=subprocess.run([sys.executable,str(pathlib.Path(candidate.__file__)),str(root),str(stdout),str(stderr)],
                                  env={**os.environ,key:value},text=True,capture_output=True)
            assert result.returncode==2 and 'unexpected role override' in result.stderr;refused+=1
print(f'PASS SYNTHETIC matrix119 envelope/{accepted} accepted DATA+cross-language+CLI cases/{refused} malformed rows,files,roles refusals/all714 rows missing+duplicate/4 states/invalid telemetry still preserves END/UNCHANGED first3 numerical failure exit1;NO model,physical timing,authentic STOP,Work or speed proof')
