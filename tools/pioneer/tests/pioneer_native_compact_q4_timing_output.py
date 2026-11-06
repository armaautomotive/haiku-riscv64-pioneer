#!/usr/bin/env python3
"""Strict107 timing receipts;read-only DATA checks,NEVER cleanup authority."""
import json
import pathlib
import re
import sys
from pioneer_native_compact_q4_stream_output import BANNERS,RESTORE,geometry,reference,returned
from pioneer_native_compact_q4_stream_parent_output import health,metadata,QUIESCED,RELOADED

FINAL='PASS_NATIVE_COMPACT_Q4_STREAM samples2 exact_words128 clean_ends2;DEBUG_REFERENCE_NOT_WORK_NO_FULL_MODEL_QUALITY_OR_SPEED_CLAIM'
PARENT_FINAL='NATIVE_COMPACT_Q4_TIMING_KNOWN_CLEAN_NO_MODEL_TPS_CLAIM'
START='COMPACT_Q4_TIMING_SOURCE_AUTH_AND_TWO_FIXED_BANDS_STARTED_WAIT_SAME_PARENT'
NORMAL='2026102100010012';NATIVE='2026110600010107'
SAMPLES=(0,6)

def summary(sample,polls,paired=False):
    tensor,first,width=geometry(sample);blocks=width//256
    return (f'PASS_NATIVE_COMPACT_Q4_STREAM_SAMPLE sample{sample} tensor{tensor} first{first} width{width} phases4 dual_jobs2 exact_words64 polls{polls}'
            f' generation{45 if paired else 43} saved_l126908 saved_gddr{blocks*1280} requests{2*blocks*26} flits{2*blocks*80}'+RESTORE)

def verify(text,paired=False):
    assert type(paired) is bool
    if not isinstance(text,str) or not text.endswith('\n') or len(text)>65536 or '\r' in text or '\0' in text:
        raise ValueError('timing child framing')
    lines=text.splitlines()
    if len(lines)!=137 or tuple(lines[:2])!=BANNERS or lines[-1]!=FINAL:raise ValueError('timing child complete envelope')
    golden=reference();index=2;polls=[];records=[]
    for sample in SAMPLES:
        for job in range(2):
            for row in range(32):
                if lines[index]!=returned(sample,job,row,golden[sample,row]):raise ValueError('timing child numerical/reference/order')
                index+=1
        match=re.search(r' polls([1-9][0-9]*) generation',lines[index])
        if not match or int(match[1])>20000 or lines[index]!=summary(sample,int(match[1]),paired):raise ValueError('timing child closed journal/epoch')
        polls.append(int(match[1]));index+=1
        for job in range(2):
            prefix=f'NATIVE_COMPACT_Q4_TIMING sample{sample} width{geometry(sample)[2]} pass{job} '
            if not lines[index].startswith(prefix):raise ValueError('timing record order/geometry')
            body=lines[index][len(prefix):];index+=1
            match=re.fullmatch(r'state1 start([1-9][0-9]*) end([1-9][0-9]*) cycles([1-9][0-9]*);CARD_CYCLES_ONLY_NO_FREQUENCY_ASSUMPTION',body)
            record={'sample':sample,'pass':job}
            if match:
                start,end,cycles=map(int,match.groups())
                if not 0<start<end<2**64 or cycles!=end-start:raise ValueError('invalid cycle DATA;not a cleanup decision')
                record.update(state=1,start=start,end=end,cycles=cycles)
            elif body in ('state0;UNAVAILABLE_NO_TIMING_OR_CLEANUP_AUTHORITY','state2;UNAVAILABLE_NO_TIMING_OR_CLEANUP_AUTHORITY'):
                record.update(state=int(body[5]),start=None,end=None,cycles=None)
            else:raise ValueError('timing record framing')
            records.append(record)
    if index!=len(lines)-1:raise ValueError('extra timing records')
    return {'samples':2,'words':128,'ends':2,'generation':45 if paired else 43,'polls':polls,'timing':records,
            'scope':'receipt DATA only;no process-exit/native/hardware/clock-frequency/model-TPS authority'}

def verify_parent(text,paired=False):
    assert type(paired) is bool
    if not isinstance(text,str) or not text.endswith('\n') or len(text)>75000 or '\r' in text or '\0' in text:raise ValueError('timing parent framing')
    lines=text.splitlines()
    if len(lines)!=173:raise ValueError('timing parent complete envelope')
    prefix,suffix=lines[:18],lines[-18:];health(prefix[1:10]);health(suffix[8:17])
    if prefix[1]!=suffix[8]:raise ValueError('kernel changed')
    before,native,after=(44,45,46) if paired else (42,43,44)
    tag='2026110600010108' if paired else NATIVE
    start=START.replace('Q4_TIMING','Q4_PAIR_ISSUE') if paired else START
    final=PARENT_FINAL.replace('Q4_TIMING','Q4_PAIR_ISSUE') if paired else PARENT_FINAL
    if prefix[0]!=metadata(NORMAL,before) or prefix[10:]!=[metadata(NORMAL,before),metadata(NORMAL,before,1),QUIESCED,
        metadata(tag,native),RELOADED,metadata(tag,native),metadata(tag,native),start]:raise ValueError('timed admission/swap order')
    if suffix[:8]!=[metadata(tag,native),metadata(tag,native),metadata(tag,native,1),QUIESCED,
        metadata(NORMAL,after),RELOADED,metadata(NORMAL,after),metadata(NORMAL,after)] or suffix[-1]!=final:
        raise ValueError('timed clean restore order')
    return dict(verify('\n'.join(lines[18:-18])+'\n',paired),restored_generation=after)

def fixture(unavailable=False,paired=False):
    words=reference();lines=list(BANNERS)
    for sample in SAMPLES:
        for job in range(2):
            for row in range(32):lines.append(returned(sample,job,row,words[sample,row]))
        lines.append(summary(sample,2,paired))
        for job in range(2):
            prefix=f'NATIVE_COMPACT_Q4_TIMING sample{sample} width{geometry(sample)[2]} pass{job} '
            lines.append(prefix+('state2;UNAVAILABLE_NO_TIMING_OR_CLEANUP_AUTHORITY' if unavailable else
                f'state1 start{1000+job*3000} end{3000+job*3000} cycles2000;CARD_CYCLES_ONLY_NO_FREQUENCY_ASSUMPTION'))
    return '\n'.join(lines+[FINAL])+'\n'

def self_test(paired=False):
    good=fixture(paired=paired);lines=good.splitlines();assert len(lines)==137 and len(verify(good,paired)['timing'])==4
    assert all(row['cycles'] is None for row in verify(fixture(True,paired),paired)['timing'])
    bad=[good[:-1],good+'NOISE\n',good.replace('\n','\r\n'),good+'\0',good.replace('generation45' if paired else 'generation43','generation37'),
         good.replace('cycles2000','cycles2001'),good.replace('start1000','start0'),good.replace('end3000','end1000'),
         good.replace('state1','state3'),good.replace('end3000','end18446744073709551616')]
    for index,line in enumerate(lines):
        bad += ['\n'.join(lines[:index]+lines[index+1:])+'\n','\n'.join(lines[:index]+[line]+lines[index:])+'\n']
        if line.startswith('NATIVE_COMPACT_Q4_STREAM_RETURNED_WORD'):
            changed=re.sub(r'got([0-9a-f]{8}) expected\1','got00000000 expected00000000',line)
            bad.append('\n'.join(lines[:index]+[changed]+lines[index+1:])+'\n')
    for candidate in bad:
        try:verify(candidate,paired)
        except ValueError:continue
        raise AssertionError('corrupt timing child accepted')
    print(f'PASS_TIMED_Q4_CHILD fixtures2 refusals{len(bad)} exact128FractionWords4clockRecords unavailableNoInventedCycles;NO_HARDWARE_AUTHORITY')

if __name__=='__main__':
    paired=sys.argv[-1:]==['--pair-issue']
    if paired:sys.argv.pop()
    if sys.argv[1:]==['--self-test']:self_test(paired)
    elif len(sys.argv)==3 and sys.argv[1] in ('--check-child','--check-parent'):
        reader=verify_parent if sys.argv[1]=='--check-parent' else verify
        print(json.dumps(reader(pathlib.Path(sys.argv[2]).read_text(),paired),indent=2,sort_keys=True))
    else:raise SystemExit('Use --self-test or --check-child/--check-parent FILE;READ ONLY')
