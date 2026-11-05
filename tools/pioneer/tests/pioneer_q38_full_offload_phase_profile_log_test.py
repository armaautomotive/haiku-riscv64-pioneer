#!/usr/bin/env python3
"""Synthetic card-clock suffixes only;never physical measurements."""
import pioneer_q38_full_offload_rate_log_test as legacy
from parse_pioneer_q38_full_offload_rate_log import *
base=legacy.fixture().replace('18500.4','19522.4')
begin=1<<40;committed=51
segments=[1]+[committed*v for v in (64,48,16,1,1,1,1,1)]+[1]
cycles=[n*19 for n in segments];end=begin+sum(cycles)
header=PREFIX+f'Full-card phase profile valid1 committed51 begin_cycles{begin} end_cycles{end} state2 error0;AFTER_CLEAN_END,clock DATA,no frequency assumption.'
buckets=[PREFIX+f'Full-card phase profile bucket{i} segments{n} cycles{c};AFTER_CLEAN_END,clock DATA,no frequency assumption.' for i,(n,c) in enumerate(zip(segments,cycles))]
def insert(rows):
 lines=base.splitlines();return '\n'.join(lines[:-3]+rows+lines[-3:])+'\n'
good=insert([header,*buckets]);opts={'prepared_q4':True,'phase_profile':True}
result=parse_complete(good,**opts);profile=result['on_card_phase_profile']
assert profile['valid'] and profile['total_cycles']==sum(cycles) and len(profile['buckets'])==10
assert [row['cycles'] for row in profile['buckets']]==cycles and not profile['physical_clock_frequency_assumed']
invalid=header.replace('valid1','valid0').replace('state2 error0','state3 error1')
assert not parse_complete(insert([invalid]),**opts)['on_card_phase_profile']['valid']
refused=0
def bad(text,stderr='',**options):
 global refused
 try:parse_complete(text,stderr,**options)
 except (InvalidLog,ValueError):refused+=1
 else:raise AssertionError('accepted invalid/missing/unselected clock suffix')
bad(good,prepared_q4=True);bad(good,phase_profile=True);bad(base,**opts)
for flag in (None,0,1,'true'):bad(good,prepared_q4=True,phase_profile=flag)
for at in range(11):
 rows=[header,*buckets];del rows[at];bad(insert(rows),**opts)
for at in range(10):
 rows=buckets[:];rows[at:at+1]=[rows[at],rows[at]];bad(insert([header,*rows]),**opts)
 rows=buckets[:];rows[at]=rows[at].replace(f'bucket{at}',f'bucket{(at+1)%10}');bad(insert([header,*rows]),**opts)
 rows=buckets[:];rows[at]=rows[at].replace(f'segments{segments[at]}',f'segments{segments[at]+1}');bad(insert([header,*rows]),**opts)
 for value in (0,cycles[at]+1,2**64):
  rows=buckets[:];rows[at]=rows[at].replace(f'cycles{cycles[at]}',f'cycles{value}');bad(insert([header,*rows]),**opts)
for old,new in (('valid1','valid0'),('committed51','committed50'),('state2','state3'),('error0','error1'),
 ('begin_cycles'+str(begin),'begin_cycles'+str(end)),('end_cycles'+str(end),'end_cycles'+str(end+1)),
 ('begin_cycles'+str(begin),'begin_cycles'+str(2**64))):
 bad(insert([header.replace(old,new),*buckets]),**opts)
bad(insert([invalid,*buckets]),**opts)
for at in range(0,len(good),23):bad(good[:at],**opts)
bad(good+'extra\n',**opts);bad(good,'synthetic stderr',**opts)
assert parse_complete(base,prepared_q4=True)['answer_hex']==result['answer_hex']
print(f'PASS_PHASE_PROFILE_LOG complete10_buckets_exact_counts_cycles_sum/no_frequency_guess/invalid_timing_available_for_diagnosis/{refused}_mode_order_count_cycle_truncation_refusals;SYNTHETIC_NO_HARDWARE_OR_TPS')
