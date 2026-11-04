#!/usr/bin/env python3
"""Synthetic complete logs only;no model or hardware measurement."""
from parse_pioneer_q38_full_offload_rate_log import *

def fixture(available=True):
    counts=[0,20,51] if available else [0,20,28]
    rows=[AUTH,READY,'FULL_OFFLOAD_PROGRESS id2 value1000 Full-card Qwen27B upload 18500.4/18500.4 MiB. No CPU inference.',RUNNING]
    rows += [PREFIX+f'Full-card count committed{n} prompts20 limit51;read-only DATA,not a terminal or cleanup receipt.' for n in counts]
    if available:
        rows.append(PREFIX+'Full-card observed count rate sampled31 first20 last51 elapsed_min_us999990 elapsed_max_us1000010 lower_tps30.999690003 upper_tps31.000310003 observations100 first_begin_us1000 first_end_us1010 last_begin_us1001000 last_end_us1001010;AFTER_CLEAN_END,post-prompt count window,not isolated latency or broad quality.')
    else:rows.append(UNAVAILABLE)
    tokens=32 if available else 8
    rows += [f'FULL_OFFLOAD_RATE_DONE P100A full-offload candidate: {tokens} output tokens; prompt+generation 2.000s,total 4.000s. All inference on card; CPU vocabulary/text only. Clean END; weights released. Not a warm-token speed measurement.',
             'FULL_OFFLOAD_RATE_ANSWER bytes3 hex=310a32',
             f'PASS_FULL_OFFLOAD_RATE_JOB clean_worker_exit1 rate_available{int(available)} output_tokens{tokens};CPU_INFERENCE_CONTEXTS0;POST_PROMPT_COUNT_WINDOW_NOT_BROAD_QUALITY_OR_ISOLATED_LATENCY']
    return '\n'.join(rows)+'\n'
good=fixture();short=fixture(False)
assert parse_complete(good)['rate_available'] and not parse_complete(short)['rate_available']
assert parse_complete(good)['answer_hex']=='310a32'
refused=0
def bad(text,stderr=''):
    global refused
    try:parse_complete(text,stderr)
    except (InvalidLog,ValueError):refused+=1
    else:raise AssertionError(('accepted',text,stderr))
for index in range(len(good.splitlines())):
    lines=good.splitlines();del lines[index];changed='\n'.join(lines)+'\n'
    # A transient pre-prompt zero may be missed by a fast observer. The
    # qualified first/last warm endpoints and whole upload/end stay required.
    if index==4:assert parse_complete(changed)['rate_available']
    else:bad(changed)
for old,new in [('sampled31','sampled30'),('first20','first21'),('last51','last50'),
                ('elapsed_min_us999990','elapsed_min_us999989'),('elapsed_max_us1000010','elapsed_max_us1000009'),
                ('lower_tps30.999690003','lower_tps32.999690003'),('upper_tps31.000310003','upper_tpsnan'),
                ('first_begin_us1000','first_begin_us1001'),('last_end_us1001010','last_end_us1001009'),
                ('observations100','observations2'),('committed51','committed19'),('bytes3','bytes4'),
                ('hex=310a32','hex=310032'),('rate_available1','rate_available0'),('output_tokens32','output_tokens31')]:
    assert old in good;bad(good.replace(old,new,1))
for text in (good[:-1],good+'extra\n',good.replace('\n','\r\n'),good.replace('hex=310a32','hex=310A32'),
             good.replace(PREFIX+'Full-card count committed20 prompts20 limit51;read-only DATA,not a terminal or cleanup receipt.\n',
                          (PREFIX+'Full-card count committed20 prompts20 limit51;read-only DATA,not a terminal or cleanup receipt.\n')*2)):
    bad(text)
bad(good,'synthetic stderr')
for index in range(0,len(good),17):bad(good[:index])
print(f'PASS independent full-card rate whole-log reader:2 valid available/unavailable fixtures/{refused} truncation,missing,ordering,count,rate,timestamp,hex,stderr refusals;synthetic ONLY,NO physical result or cleanup authority')
