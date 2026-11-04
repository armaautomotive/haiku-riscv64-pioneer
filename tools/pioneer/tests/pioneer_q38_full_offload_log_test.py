#!/usr/bin/env python3
"""Strict closed-transcript reader over synthetic text ONLY;never hardware."""
from parse_pioneer_q38_full_offload_log import AUTH,READY,RUNNING,ANSWER,FINAL,TOTAL,InvalidLog,parse_complete

upload0='FULL_OFFLOAD_PROGRESS id2 value0 Full-card Qwen27B upload 1.0/18500.4 MiB. No CPU inference.'
upload1='FULL_OFFLOAD_PROGRESS id2 value1000 Full-card Qwen27B upload 18500.4/18500.4 MiB. No CPU inference.'
done='FULL_OFFLOAD_DONE P100A full-offload candidate: 4 output tokens; prompt+generation 123.456s,total 2000.123s. All inference on card; CPU vocabulary/text only. Clean END; weights released. Not a warm-token speed measurement.'
lines=[AUTH,READY,upload0,upload1,RUNNING,done,ANSWER,FINAL]
good='\n'.join(lines)+'\n';result=parse_complete(good)
assert result['output_tokens']==4 and result['uploaded_bytes']==TOTAL and result['upload_events']==2
assert result['warm_token_rate'] is None and result['timing_fields_valid'] and not result['execution_or_cleanup_authority_from_log']
# Real-formatted progress across the entire original4096-aligned upload,
# including small first pages and both rounding sides near the final page.
pages=TOTAL//4096
positions=sorted({4096,(pages-1)*4096,TOTAL,*[(pages*i//1000)*4096 for i in range(1,1000)]})
upload_lines=[f'FULL_OFFLOAD_PROGRESS id2 value{b*1000//TOTAL} Full-card Qwen27B upload {b/1048576:.1f}/18500.4 MiB. No CPU inference.' for b in positions]
many='\n'.join([AUTH,READY,*upload_lines,RUNNING,done,ANSWER,FINAL])+'\n'
assert parse_complete(many)['upload_events']==len(positions)
refused=0
def refuse(text,stderr=''):
    global refused
    try:parse_complete(text,stderr)
    except InvalidLog:refused+=1;return
    raise AssertionError('bad transcript accepted')
for index in range(len(lines)):
    if index==2:
        # Throttled progress may omit early rows or repeat rounded values.
        # This does NOT remove the required final full upload/execution/end.
        assert parse_complete('\n'.join(lines[:index]+lines[index+1:])+'\n')['upload_events']==1
        assert parse_complete('\n'.join(lines[:index]+[lines[index]]+lines[index:])+'\n')['upload_events']==3
    else:
        refuse('\n'.join(lines[:index]+lines[index+1:])+'\n')
        refuse('\n'.join(lines[:index]+[lines[index]]+lines[index:])+'\n')
    for other in range(index+1,len(lines)):
        changed=lines[:];changed[index],changed[other]=changed[other],changed[index]
        refuse('\n'.join(changed)+'\n')
for index in range(len(good)):
    if good[index]=='\n':continue
    refuse(good[:index]+'~'+good[index+1:])
for old,new in [('id2','id3'),('CPU inference OFF','CPU inference ON'),('All inference on card','CPU attention/state'),
                ('clean_worker_exit1','clean_worker_exit0'),('CPU_INFERENCE_CONTEXTS0','CPU_INFERENCE_CONTEXTS1'),
                ('4 output tokens','3 output tokens'),('bytes14','bytes13'),('Hello! How can','Hi! How can'),
                ('value1000','value999'),('18500.4 MiB. No','18500.3 MiB. No')]:
    refuse(good.replace(old,new))
for suffix in ('extra\n','\n','FAIL_ANYTHING\n'):refuse(good+suffix)
refuse(good[:-1]);refuse(good.replace('\n','\r\n'));refuse(good+'\0');refuse(good,'warning\n')
refuse(good,None)
refuse(good.replace(upload1,upload0))
# Bad timing DATA must not fabricate a speed or imply hardware cleanup failed.
for compute,total in [('0.000','2000.123'),('123.456','1.000')]:
    changed=good.replace('123.456s,total 2000.123s',compute+'s,total '+total+'s')
    parsed=parse_complete(changed);assert not parsed['timing_fields_valid'] and parsed['warm_token_rate'] is None
print(f'PASS complete full-card log reader:4 valid including{len(positions)}-row full upload/{refused} refusals/2 invalid-timing DATA cases;synthetic ONLY,no inference/cleanup authority or token-rate extrapolation')
