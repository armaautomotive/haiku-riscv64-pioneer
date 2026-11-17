#!/usr/bin/env python3
"""Exact prepared upload accounting;synthetic transcripts,not measurements."""
import pioneer_q38_full_offload_rate_log_test as legacy
from parse_pioneer_q38_full_offload_rate_log import *
from parse_pioneer_q38_full_offload_log import MIB
good=legacy.fixture().replace('18500.4','19522.4')
short=legacy.fixture(False).replace('18500.4','19522.4')
assert parse_complete(good,prepared_q4=True)['uploaded_bytes']==PREPARED_TOTAL
assert parse_complete(good,prepared_q4=True)['rate_available']
assert not parse_complete(short,prepared_q4=True)['rate_available']
refused=0
def bad(text,stderr='',**options):
 global refused
 try:parse_complete(text,stderr,**options)
 except (InvalidLog,ValueError):refused+=1
 else:raise AssertionError('accepted malformed/cross-layout transcript')
bad(good);bad(legacy.fixture(),prepared_q4=True)
for flag in (0,1,None,'true'):bad(good,prepared_q4=flag)
for old,new in [('value1000','value999'),('19522.4/19522.4','19522.3/19522.4'),
 ('/19522.4','/19522.3'),('sampled31','sampled30'),('first20','first21'),
 ('elapsed_min_us999990','elapsed_min_us999989'),('lower_tps30.999690003','lower_tps32.999690003'),
 ('observations100','observations2'),('hex=310a32','hex=310032'),('clean_worker_exit1','clean_worker_exit0')]:
 assert old in good;bad(good.replace(old,new,1),prepared_q4=True)
for at in range(0,len(good),17):bad(good[:at],prepared_q4=True)
bad(good,'synthetic stderr',prepared_q4=True)
marks=READINESS_STDERR.splitlines(keepends=True)
assert parse_complete(good,READINESS_STDERR,prepared_q4=True,
                      prepared_readiness_diagnostic=True)['prepared_readiness_markers']==24
bad(good,READINESS_STDERR,prepared_q4=True)  # old modes still require empty stderr
bad(legacy.fixture(),READINESS_STDERR,prepared_readiness_diagnostic=True)
for diagnostic in ('', ''.join(marks[1:]), ''.join(marks[:-1]),
                   marks[0]+READINESS_STDERR, ''.join([marks[1],marks[0],*marks[2:]]),
                   READINESS_STDERR.replace('/4678','/4679',1),
                   READINESS_STDERR.replace('SOURCE_PART','FRAGMENT',1),
                   READINESS_STDERR+'fatal diagnostic\n', READINESS_STDERR[:-1]):
 bad(good,diagnostic,prepared_q4=True,prepared_readiness_diagnostic=True)
bad(good,READINESS_STDERR,prepared_q4=True,prepared_readiness_diagnostic=1)
bad(good.replace('clean_worker_exit1','clean_worker_exit0'),READINESS_STDERR,
    prepared_q4=True,prepared_readiness_diagnostic=True)
positions=list(range(1048576,PREPARED_TOTAL,1048576))+[PREPARED_TOTAL]
uploads=[f'FULL_OFFLOAD_PROGRESS id2 value{b*1000//PREPARED_TOTAL} Full-card Qwen27B upload {b/MIB:.1f}/19522.4 MiB. No CPU inference.' for b in positions]
lines=good.splitlines();many='\n'.join(lines[:2]+uploads+lines[3:])+'\n'
assert parse_complete(many,prepared_q4=True)['upload_rows']==len(positions)
coarse=many.replace(uploads[3],uploads[3]+'\n'+uploads[3])
assert parse_complete(coarse,prepared_q4=True)['upload_rows']==len(positions)+1
for malformed in (many.replace(uploads[-1],uploads[-2]),many.replace(uploads[-1],uploads[-1]+'\n'+uploads[-1])):
 bad(malformed,prepared_q4=True)
print(f'PASS prepared rate reader exact20GB layout/legacy regression/{len(uploads)} increasing upload rows/{refused} truncation,mode,bytes,rate,exit,stderr refusals;SYNTHETIC_NO_HARDWARE_OR_TPS')
