#!/usr/bin/env python3
"""Inspect a closed raw resident rate capture; never contacts the Pioneer."""
from decimal import Decimal
from hashlib import sha256
import json
from pathlib import Path
import re
import sys

from compare_qwen38_head_csr_rates import EXPECTED_ANSWER_HEX
from parse_pioneer_q38_full_offload_rate_log import (
    AUTH, COUNT, NUMBER, PREFIX, RATE, RUNNING, TOTAL, UNAVAILABLE, InvalidLog,
    count_window, require, stable_text, upload_prefix,
)

READY = 'RESIDENT_READY_NO_CPU_INFERENCE;ONE_WORKER_TWO_MATCHED_REQUESTS'
FINAL = 'PASS_RESIDENT_TWO_REQUEST one_upload1 model_end1 worker_exit0 CPU_INFERENCE_CONTEXTS0;NOT_BROAD_QUALITY'
CLOSE = re.compile(r'RESIDENT_REQUEST_CLOSED request([12]) uploads([01]) tokens32 host_seconds('
                   + NUMBER + r'\.[0-9]{3}) rate_available([01]) P100A full-offload resident candidate: '
                   r'32 output tokens; prompt\+generation (' + NUMBER + r'\.[0-9]{3})s,total ('
                   + NUMBER + r'\.[0-9]{3})s\. All inference on card; CPU vocabulary/text only\. '
                   r'Request closed cleanly; weights retained until explicit model END\. Not a warm-token speed measurement\.')


def inspect(text):
    require(isinstance(text,str) and len(text)<=16*1024*1024 and text.isascii() and text.endswith('\n')
            and '\r' not in text and '\0' not in text,'whole ASCII/LF resident transcript required')
    lines=text[:-1].split('\n')
    require(len(lines)>=13 and lines[:2]==[AUTH,READY] and lines[-1]==FINAL,
            'exact resident authentication, ready and verified model END required')
    cursor,uploads=upload_prefix(lines,2)
    requests=[]
    for index in (1,2):
        prefix=f'FULL_OFFLOAD_PROGRESS id{index+1} value4294967295 '
        def regex_prefix(pattern):
            return pattern.replace(re.escape(PREFIX),re.escape(prefix),1)

        require(cursor<len(lines) and lines[cursor]==RUNNING.replace(PREFIX,prefix,1),
                'exact request start; a warm request must not upload')
        cursor,counts,prompts,bounds=count_window(
            lines,cursor+1,count=re.compile(regex_prefix(COUNT.pattern)),
            rate_pattern=re.compile(regex_prefix(RATE.pattern).replace(
                'AFTER_CLEAN_END,post-prompt count window,',
                'AFTER_CLEAN_REQUEST_CLOSE,model reservation retained;',1)),
            unavailable=UNAVAILABLE.replace(PREFIX,prefix,1).replace(
                'AFTER_CLEAN_END,no extrapolation.', 'AFTER_CLEAN_REQUEST_CLOSE,no model END or extrapolation.',1))
        require(prompts==31 and counts[-1]==62 and cursor+1<len(lines),'exact resident count geometry')
        close=CLOSE.fullmatch(lines[cursor])
        require(close and int(close[1])==index and int(close[2])==int(index==1)
                and int(close[4])==int(bounds is not None),'request closure, upload or rate flag disagrees')
        host,compute,total=map(Decimal,(close[3],close[5],close[6]))
        require(compute>0 and total>=compute and host+Decimal('.001')>=total,'resident host/compute time bounds')
        if bounds:
            require(Decimal(bounds['maximum_us'])<=(compute+Decimal('.001'))*1000000,
                    'resident observed window outside compute interval')
        require(lines[cursor+1]==f'RESIDENT_ANSWER request{index} bytes32 hex={EXPECTED_ANSWER_HEX}',
                'resident answer differs from the pinned public counting answer')
        requests.append({'request':index, 'weight_upload_rows':uploads if index==1 else 0,
                         'uploaded_bytes':TOTAL if index==1 else 0,
                         'compute_and_prefill_seconds':str(compute), 'worker_total_seconds':str(total),
                         'host_seconds':str(host), 'observed_post_prompt_count_rate_bounds':bounds})
        cursor+=2
    require(cursor==len(lines)-1,'extra resident output or more requests')
    first,second=(r['observed_post_prompt_count_rate_bounds'] for r in requests)
    if first and second:
        require(first['raw_endpoints_us'][-1]<second['raw_endpoints_us'][0],
                'warm count timestamps precede or overlap the cold request')
    return {'scope':'one exact 32-output prompt run twice in a single retained-weight session',
            'transcript_sha256':sha256(text.encode('ascii')).hexdigest(),
            'requests':requests, 'total_uploaded_bytes':TOTAL, 'model_end_marker':True,
            'broad_model_quality':False, 'execution_or_cleanup_authority_from_log':False,
            'requires_independent_package_identity_and_machine_postflight':True}


def main():
    if len(sys.argv)!=2:
        raise SystemExit('use CLOSED_RESIDENT_CAPTURE')
    stage=Path(sys.argv[1])
    try:
        for filename in ('rate.exit','parent.exit','rate.stderr','parent.stderr'):
            require(stable_text(stage/filename)==('0\n' if filename.endswith('.exit') else ''),
                    'nonzero exit or unexpected stderr: '+filename)
        verified=inspect(stable_text(stage/'rate.stdout'))
    except (OSError,InvalidLog,ValueError) as error:
        raise SystemExit('REFUSE_VECTOR_RESIDENT_RATE '+str(error))
    print(json.dumps(verified,sort_keys=True))


if __name__=='__main__':
    main()
