#!/usr/bin/env python3
"""Compare closed v3/v4 profiles as measured cycle shares, not speedups."""
from decimal import Decimal
from hashlib import sha256
import json
from pathlib import Path
import re
import sys

from compare_qwen38_head_csr_rates import EXPECTED_ANSWER_HEX, InvalidLog, parse_complete, require, stable_text

GROUPS = (
    ('matrix_calls', ('matrix_prepare', 'matrix_dispatch', 'matrix_gather_wait', 'matrix_finish'), ('matrix_total',)),
    ('recurrent_outside_matrix', ('recurrent_outside_matrix_calls',), ('recurrent_other', 'conv_stage', 'gdn_stage')),
    ('dense_outside_matrix', ('dense_outside_matrix_calls',), ('dense_other', 'dense_attention_prepare_attention_kv')),
    ('head', ('head_normalize', 'head_matrix'), ('head_normalize', 'head_matrix')),
    ('metadata', ('metadata',), ('metadata',)),
    ('entry_feedback_commit_publication', ('generation_entry', 'feedback', 'state_commit', 'publication'),
     ('generation_entry', 'feedback', 'state_commit', 'publication')),
)


def read_profile(stage, version):
    require(type(version) is int and version in (3, 4), 'explicit v3 or v4 capture only')
    for name in ('rate.exit', 'parent.exit'):
        require(stable_text(stage/name) == '0\n', f'v{version} {name} is not exactly zero')
    state=stable_text(stage/'state')
    require(state == 'FULL_CARD_RATE32_CLOSED_STABLE_RESTORED\n' if version == 3 else
            re.fullmatch(r'STABLE041_GEN[1-9][0-9]*_RESTORED\n',state), f'v{version} stage lacks closed marker')
    stdout=stable_text(stage/'rate.stdout')
    result=parse_complete(stdout,stable_text(stage/'rate.stderr'),startup_timing=True,
                          fast_decode_profile=version==3,layer_split_profile=version==4)
    profile=result['on_card_fast_decode_profile' if version==3 else 'on_card_layer_split_profile']
    rate=result['observed_post_prompt_count_rate_bounds']
    require(profile['valid'] and result['rate_available'] and result['answer_hex']==EXPECTED_ANSWER_HEX,
            f'v{version} requires a valid timing profile, rate and exact counting answer')
    require((result['output_tokens'],result['prompts'],rate['sampled_commit_steps'],rate['first_committed'],
             rate['last_committed'],profile['profiled_positions'],profile['window_start'],profile['last_position'])
            == (32,31,31,31,62,31,31,61), f'v{version} is not the matched 31-position generation request')
    return sha256(stdout.encode('ascii')).hexdigest(),result,profile


def compare(v3_dir,v4_dir):
    require(v3_dir.resolve()!=v4_dir.resolve(),'v3 and v4 must be independent captures')
    (s3,r3,p3),(s4,r4,p4)=read_profile(v3_dir,3),read_profile(v4_dir,4)
    require(s3!=s4 and r3['uploaded_bytes']==r4['uploaded_bytes'],'duplicate transcript or unequal raw model bytes')
    profiles=(p3,p4)
    cycles=tuple({row['name']:row['cycles'] for row in p['buckets'] if row['name']!='final_stop'} for p in profiles)
    body=tuple(p['generation_body_cycles'] for p in profiles)
    mapped=[[],[]]
    groups=[]
    for label,*names in GROUPS:
        values=tuple(sum(c[n] for n in selected) for c,selected in zip(cycles,names))
        for seen,selected in zip(mapped,names):
            seen.extend(selected)
        groups.append({'group':label,'v3_cycles':values[0],'v4_cycles':values[1],
                       'v3_body_percent':f'{Decimal(values[0])*100/body[0]:.6f}',
                       'v4_body_percent':f'{Decimal(values[1])*100/body[1]:.6f}'})
    require(all(len(m)==len(set(m)) and set(m)==set(c) and sum(c.values())==b
                for m,c,b in zip(mapped,cycles,body)),'group mapping must account for every exclusive non-STOP bucket once')
    ranked=[]
    for label,n in sorted(cycles[1].items(),key=lambda x:(-x[1],x[0])):
        ranked.append({'bucket':label,'cycles':n,'body_percent':f'{Decimal(n)*100/body[1]:.6f}',
                       'mean_cycles_per_position':f'{Decimal(n)/p4["profiled_positions"]:.3f}',
                       'ideal_body_factor_if_twice_as_fast':f'{Decimal(2*body[1])/(2*body[1]-n):.6f}'})
    return {'scope':'matched 31 generation positions; exclusive controller cycles with final STOP excluded',
            'transcript_sha256':{'v3':s3,'v4':s4},'body_cycles':dict(zip(('v3','v4'),body)),
            'final_stop_cycles':{v:p['final_stop_cycles'] for v,p in zip(('v3','v4'),profiles)},
            'common_group_shares':groups,'ranked_v4_buckets':ranked,
            'host_rate_observations_separate_from_profile':{
                v:[r['observed_post_prompt_count_rate_bounds'][k] for k in ('lower_tps','upper_tps')]
                for v,r in zip(('v3','v4'),(r3,r4))},
            'host_startup_observations_separate_from_profile':{
                v:r['host_startup_observations'] for v,r in zip(('v3','v4'),(r3,r4))},
            'limitation':'bucket scopes include control and waits; ideal factor is not measured or predicted token rate',
            'requires_independent_package_identity_and_machine_postflight':True}


def main():
    if len(sys.argv)!=3:
        raise SystemExit('use CLOSED_V3_CAPTURE CLOSED_V4_CAPTURE')
    try:
        result=compare(*map(Path,sys.argv[1:]))
    except (OSError,InvalidLog,ValueError) as exc:
        raise SystemExit('REFUSE_LAYER_PROFILE_COMPARISON '+str(exc))
    print(json.dumps(result,sort_keys=True))


if __name__=='__main__':
    main()
