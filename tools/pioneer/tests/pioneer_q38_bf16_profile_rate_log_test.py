#!/usr/bin/env python3
"""Explicit fourteen-phase BF16 log grammar; synthetic data, not performance."""
import pioneer_q38_full_offload_phase_profile_log_test as old_phase
import pioneer_q38_full_offload_rate_log_test as legacy
from parse_pioneer_q38_full_offload_rate_log import *

base=legacy.fixture();committed=51;begin=1<<40
segments=[1,64*committed,432*committed,128*committed,*([committed]*5),1,*([496*committed]*4)]
cycles=[n*19 for n in segments];end=begin+sum(cycles)
header=PREFIX+f'Full-card phase profile valid1 committed51 begin_cycles{begin} end_cycles{end} state2 error0;AFTER_CLEAN_END,clock DATA,no frequency assumption.'
buckets=[PREFIX+f'Full-card phase profile bucket{i} segments{n} cycles{c};AFTER_CLEAN_END,clock DATA,no frequency assumption.' for i,(n,c) in enumerate(zip(segments,cycles))]
def insert(rows):
 lines=base.splitlines();return '\n'.join(lines[:-3]+rows+lines[-3:])+'\n'
good=insert([header,*buckets]);opts={'phase_profile':True,'bf16_profile':True}
result=parse_complete(good,**opts);profile=result['on_card_phase_profile']
assert result['uploaded_bytes']==TOTAL and result['bf16_profile'] and len(profile['buckets'])==14
assert profile['total_cycles']==sum(cycles) and not profile['physical_clock_frequency_assumed']
refused=0
def bad(text,stderr='',**options):
 global refused
 try:parse_complete(text,stderr,**options)
 except (InvalidLog,ValueError):refused+=1
 else:raise AssertionError('accepted wrong/missing/unselected BF16 profile')
bad(good);bad(good,phase_profile=True,prepared_q4=True);bad(good,bf16_profile=True)
bad(good,**opts,prepared_q4=True);bad(old_phase.good,**opts);bad(base,**opts)
for flag in (None,0,1,'true'):bad(good,phase_profile=True,bf16_profile=flag)
for at in range(15):
 rows=[header,*buckets];del rows[at];bad(insert(rows),**opts)
for at in range(14):
 rows=buckets[:];rows[at:at+1]=[rows[at],rows[at]];bad(insert([header,*rows]),**opts)
 rows=buckets[:];rows[at]=rows[at].replace(f'segments{segments[at]}',f'segments{segments[at]+1}');bad(insert([header,*rows]),**opts)
 rows=buckets[:];rows[at]=rows[at].replace(f'cycles{cycles[at]}',f'cycles{cycles[at]+1}');bad(insert([header,*rows]),**opts)
invalid=header.replace('valid1','valid0').replace('state2 error0','state3 error1')
assert not parse_complete(insert([invalid]),**opts)['on_card_phase_profile']['valid']
bad(insert([invalid,*buckets]),**opts)
for at in range(0,len(good),31):bad(good[:at],**opts)
bad(good+'extra\n',**opts);bad(good,'synthetic stderr',**opts)
print(f'PASS BF16 rate log14 ordered buckets/exact sums/original upload/old grammar preserved/{refused} mode,count,cycle,truncation refusals;SYNTHETIC_NO_HARDWARE_OR_TPS')
