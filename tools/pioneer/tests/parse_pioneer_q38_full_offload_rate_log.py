#!/usr/bin/env python3
"""Independent whole closed full-card count-rate transcript DATA checks.

Never executes,queries,cleans up or restores the accelerator. Source/parent
exit/actual restoration are separate required evidence,not granted by a log.
"""
import argparse
from decimal import Decimal
import json
import pathlib
import re
from parse_pioneer_q38_full_offload_log import AUTH,RUNNING,TOTAL,PREPARED_TOTAL,NUMBER,InvalidLog,require,stable_text,upload_prefix

READY='FULL_OFFLOAD_RATE_READY_NO_CPU_CONTEXT;REQUEST_MAX32_PUBLIC_COUNTING'
PREFIX='FULL_OFFLOAD_PROGRESS id2 value4294967295 '
COUNT=re.compile(re.escape(PREFIX)+r'Full-card count committed('+NUMBER+r') prompts('+NUMBER+r') limit('+NUMBER+r');read-only DATA,not a terminal or cleanup receipt\.')
RATE=re.compile(re.escape(PREFIX)+r'Full-card observed count rate sampled('+NUMBER+r') first('+NUMBER+r') last('+NUMBER+r') elapsed_min_us('+NUMBER+r') elapsed_max_us('+NUMBER+r') lower_tps('+NUMBER+r'\.[0-9]{9}) upper_tps('+NUMBER+r'\.[0-9]{9}) observations('+NUMBER+r') first_begin_us('+NUMBER+r') first_end_us('+NUMBER+r') last_begin_us('+NUMBER+r') last_end_us('+NUMBER+r');AFTER_CLEAN_END,post-prompt count window,not isolated latency or broad quality\.')
UNAVAILABLE=PREFIX+'Full-card rate unavailable: insufficient post-prompt window or invalid timing;AFTER_CLEAN_END,no extrapolation.'
DONE=re.compile(r'FULL_OFFLOAD_RATE_DONE P100A full-offload candidate: ('+NUMBER+r') output tokens; prompt\+generation ('+NUMBER+r'\.[0-9]{3})s,total ('+NUMBER+r'\.[0-9]{3})s\. All inference on card; CPU vocabulary/text only\. Clean END; weights released\. Not a warm-token speed measurement\.')
ANSWER=re.compile(r'FULL_OFFLOAD_RATE_ANSWER bytes('+NUMBER+r') hex=([0-9a-f]*)')
FINAL=re.compile(r'PASS_FULL_OFFLOAD_RATE_JOB clean_worker_exit1 rate_available([01]) output_tokens('+NUMBER+r');CPU_INFERENCE_CONTEXTS0;POST_PROMPT_COUNT_WINDOW_NOT_BROAD_QUALITY_OR_ISOLATED_LATENCY')
PHASE_HEADER=re.compile(re.escape(PREFIX)+r'Full-card phase profile valid([01]) committed('+NUMBER+r') begin_cycles('+NUMBER+r') end_cycles('+NUMBER+r') state('+NUMBER+r') error('+NUMBER+r');AFTER_CLEAN_END,clock DATA,no frequency assumption\.')
PHASE_BUCKET=re.compile(re.escape(PREFIX)+r'Full-card phase profile bucket('+NUMBER+r') segments('+NUMBER+r') cycles('+NUMBER+r');AFTER_CLEAN_END,clock DATA,no frequency assumption\.')
def phase_profile_suffix(lines,cursor,committed):
    require(cursor<len(lines),'complete explicit phase header required')
    header=PHASE_HEADER.fullmatch(lines[cursor]);require(header,'exact phase header');cursor+=1
    valid,count,begin,end,state,error=map(int,header.groups())
    require(count==committed and max(begin,end)<=2**64-1 and max(state,error)<=2**32-1,'phase header bounds/committed mismatch')
    result={'valid':bool(valid),'committed':count,'begin_cycles':begin,'end_cycles':end,'state':state,'error':error,
            'physical_clock_frequency_assumed':False}
    if not valid:return cursor,result
    require(state==2 and not error and begin<end,'closed valid timing header')
    buckets=[];total=0
    names=('bootstrap','metadata','recurrent_layer','dense_layer','head_normalize','head_matrix','feedback','state_commit','publication','final_stop')
    for phase,name in enumerate(names):
        require(cursor<len(lines),'complete ten-bucket timing suffix required')
        row=PHASE_BUCKET.fullmatch(lines[cursor]);require(row,'exact timing bucket');cursor+=1
        actual,segments,cycles=map(int,row.groups())
        expected=1 if phase in (0,9) else committed*({1:64,2:48,3:16}.get(phase,1))
        require(actual==phase and segments==expected and 0<cycles<=2**64-1-total,'ordered phase counts/cycle bounds')
        total+=cycles;buckets.append({'phase':phase,'name':name,'segments':segments,'cycles':cycles})
    require(total==end-begin,'whole physical cycle sum')
    for row in buckets:row['share_percent']=str(Decimal(row['cycles'])*100/Decimal(total))
    result.update(total_cycles=total,buckets=buckets)
    return cursor,result
def parse_complete(text,stderr='',*,prepared_q4=False,phase_profile=False):
    require(type(prepared_q4) is bool,'explicit prepared-layout selector required')
    require(type(phase_profile) is bool and (not phase_profile or prepared_q4),'explicit compatible phase-profile selector required')
    planned=PREPARED_TOTAL if prepared_q4 else TOTAL
    require(isinstance(stderr,str) and not stderr,'nonempty/unknown stderr')
    require(isinstance(text,str) and len(text)<=16*1024*1024 and text.isascii() and text.endswith('\n')
            and '\r' not in text and '\0' not in text,'whole ASCII/LF transcript required')
    lines=text[:-1].split('\n');require(len(lines)>=9 and lines[:2]==[AUTH,READY],'exact full-card source/ready prefix')
    cursor,uploads=upload_prefix(lines,2,planned)
    require(cursor<len(lines) and lines[cursor]==RUNNING,'exact running marker');cursor+=1
    counts=[];prompts=limit=first_warm=None
    while cursor<len(lines):
        m=COUNT.fullmatch(lines[cursor])
        if not m:break
        n,p,l=map(int,m.groups())
        require(1<=p<=96 and l==p+31 and l<=128 and n<=l,'full-card32 count bounds')
        require(not counts or (p==prompts and l==limit and n>counts[-1]),'count/geometry regression or duplicate')
        counts.append(n);require(len(counts)<=129,'count row bound');prompts,limit=p,l
        if first_warm is None and n>=p:first_warm=n
        cursor+=1
    require(counts and first_warm is not None and len(lines)-cursor>=4,'complete counter/rate/done/answer/final suffix')
    rate=RATE.fullmatch(lines[cursor]);available=rate is not None;bounds=None
    if rate:
        g=rate.groups();sampled,first,last,minimum,maximum=map(int,g[:5]);low,high=map(Decimal,g[5:7])
        observations,fb,fe,lb,le=map(int,g[7:])
        require(first==first_warm and first>=prompts and last==limit and last==counts[-1] and first<last
                and sampled==last-first and 16<=sampled<=31,'post-prompt sampled count geometry')
        require(len(counts)<=observations<=18001 and 0<=fb<=fe<lb<=le<=2**63-1
                and minimum==lb-fe and maximum==le-fb and 0<minimum<=maximum,'raw timestamp bounds')
        for value,expected in ((low,Decimal(sampled)*1000000/maximum),(high,Decimal(sampled)*1000000/minimum)):
            require(value>0 and abs(value-expected)<=Decimal('0.00000000051')+Decimal('0.000000000001')*expected,'rate arithmetic')
        require(low<=high,'inverted rate')
        bounds={'sampled_commit_steps':sampled,'first_committed':first,'last_committed':last,
                'minimum_us':minimum,'maximum_us':maximum,'lower_tps':str(low),'upper_tps':str(high),
                'observations':observations,'raw_endpoints_us':[fb,fe,lb,le]}
    else:require(lines[cursor]==UNAVAILABLE,'unavailable is explicit,never an inferred rate')
    cursor+=1;profile=None
    if phase_profile:cursor,profile=phase_profile_suffix(lines,cursor,counts[-1])
    require(len(lines)-cursor==3,'exact done/answer/final suffix,no unselected timing rows')
    done=DONE.fullmatch(lines[cursor]);answer=ANSWER.fullmatch(lines[cursor+1]);final=FINAL.fullmatch(lines[cursor+2])
    require(done and answer and final,'whole exact completion/answer/clean-worker suffix')
    tokens=int(done[1]);compute,total=map(Decimal,done.groups()[1:])
    require(tokens<=32 and compute>0 and total>=compute and int(final[1])==int(available)
            and int(final[2])==tokens and counts[-1]-prompts+1 in (tokens,tokens+1),'completion/count/timing fields')
    if bounds:require(Decimal(bounds['maximum_us'])<=(compute+Decimal('.001'))*1000000,'window outside reported compute interval')
    length=int(answer[1]);raw=bytes.fromhex(answer[2])
    require(length<=8192 and len(answer[2])==length*2 and len(raw)==length and b'\0' not in raw,'answer hex/length')
    result={'scope':'one public counting request,maximum32 outputs,full-card candidate','output_tokens':tokens,
            'answer_hex':answer[2],'answer_utf8_preview':raw.decode('utf8',errors='replace'),'prompts':prompts,
            'count_rows':len(counts),'upload_rows':uploads,'uploaded_bytes':planned,'rate_available':available,
            'observed_post_prompt_count_rate_bounds':bounds,'broad_model_quality':False,
            'execution_or_cleanup_authority_from_log':False,'isolated_token_latency':False}
    if phase_profile:result['on_card_phase_profile']=profile
    return result
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('stdout',type=pathlib.Path);parser.add_argument('stderr',type=pathlib.Path)
    parser.add_argument('--prepared-q4',action='store_true',help='require exact20470759424B prepared FFN layout,never infer mode from a log')
    parser.add_argument('--phase-profile',action='store_true',help='require the explicit complete card-clock timing suffix;requires --prepared-q4')
    args=parser.parse_args()
    try:result=parse_complete(stable_text(args.stdout),stable_text(args.stderr),prepared_q4=args.prepared_q4,phase_profile=args.phase_profile)
    except (OSError,InvalidLog,ValueError) as error:raise SystemExit('REFUSE_FULL_CARD_RATE_LOG '+str(error))
    print(json.dumps(result,sort_keys=True))
    print('PASS_COMPLETE_FULL_CARD_RATE_TRANSCRIPT_DATA_ONLY;NOT_BROAD_QUALITY_ISOLATED_LATENCY_OR_NATIVE_CLEANUP_AUTHORITY')
if __name__=='__main__':main()
