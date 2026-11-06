#!/usr/bin/env python3
"""Explicit109 startup observation grammar;synthetic only,not a speed test."""
import pioneer_q38_bf16_profile_rate_log_test as bf16
from parse_pioneer_q38_full_offload_rate_log import parse_complete,InvalidLog,PREFIX,STARTUP_UNAVAILABLE

row=PREFIX+'Full-card startup timing vocab_us1200000 setup_us1500000 upload_calls20000 upload_bytes19399024640 upload_window_us800000 upload_ioctl_us600000 first_commit_valid1 first_commit_min_us1900000 first_commit_max_us2000000 answer_ready_us3900000;AFTER_CLEAN_END,host_us,first_commit_not_streamed_text,upload_window_includes_between_call_preparation.'
opts={'phase_profile':True,'bf16_profile':True,'startup_timing':True}
def insert(rows):
    lines=bf16.good.splitlines();return '\n'.join(lines[:-3]+rows+lines[-3:])+'\n'
good=insert([row]);result=parse_complete(good,**opts)['host_startup_observations']
assert result['available'] and result['first_commit_available'] and result['request_setup_us']==1500000
assert result['first_commit_minimum_us']==1900000 and not result['first_commit_is_streamed_text']
assert not parse_complete(insert([STARTUP_UNAVAILABLE]),**opts)['host_startup_observations']['available']
missed=row.replace('first_commit_valid1 first_commit_min_us1900000 first_commit_max_us2000000',
                   'first_commit_valid0 first_commit_min_us0 first_commit_max_us0')
assert not parse_complete(insert([missed]),**opts)['host_startup_observations']['first_commit_available']
refused=0
def bad(text,**flags):
    global refused
    try:parse_complete(text,**flags)
    except (InvalidLog,ValueError):refused+=1
    else:raise AssertionError('accepted missing/unselected/malformed startup timing')
bad(good,phase_profile=True,bf16_profile=True);bad(bf16.good,**opts)
bad(good,startup_timing=True);bad(insert([row,row]),**opts)
for field,value in (
 ('vocab_us1200000','vocab_us-1'),('setup_us1500000','setup_us799999'),
 ('upload_calls20000','upload_calls0'),('upload_calls20000','upload_calls1'),
 ('upload_bytes19399024640','upload_bytes19399024641'),('upload_ioctl_us600000','upload_ioctl_us800001'),
 ('first_commit_valid1','first_commit_valid2'),('first_commit_min_us1900000','first_commit_min_us1499999'),
 ('first_commit_max_us2000000','first_commit_max_us1899999'),('answer_ready_us3900000','answer_ready_us1999999'),
 ('vocab_us1200000','vocab_us01200000'),('vocab_us1200000','vocab_us9223372036854775808')):
    bad(insert([row.replace(field,value)]),**opts)
for at in range(0,len(good),47):bad(good[:at],**opts)
bad(good+'extra\n',**opts)
print(f'PASS startup timing explicit selected log/available+unavailable+skipped-commit DATA/{refused} order,mode,bounds,truncation refusals;SYNTHETIC_NO_HARDWARE_OR_TPS')
