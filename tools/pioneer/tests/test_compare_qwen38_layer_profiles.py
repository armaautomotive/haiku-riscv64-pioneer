#!/usr/bin/env python3
"""Archived v3 plus synthetic v4; separate file/cycle/scope checks."""
from pathlib import Path
import re
import sys
from tempfile import TemporaryDirectory

here=Path(__file__).resolve().parent.parent
sys.path.insert(0,str(here))
from compare_qwen38_layer_profiles import compare, compare_v4, read_profile, InvalidLog

old=here/'benchmarks/qwen38-fast-decode-profile-physical-20261106/remote'
v3_sha,_,p3=read_profile(old,3)
assert v3_sha=='3bfd8cdc117626f3b5c82e18ee42f163c8189f5e2db81ed63d3da90c460c2a19'
assert p3['generation_body_cycles']==242102575794
rows=p3['buckets']
cycles=[row['cycles'] for row in rows[:10]]
recurrent=cycles[2]//5
attention=cycles[3]//2
cycles[2]-=2*recurrent
cycles[3]-=attention
cycles += [sum(row['cycles'] for row in rows[10:]),recurrent,recurrent,attention]
segments=[1]+[31*n for n in (64,480,144,1,1,1,1,1)]+[1]+[31*n for n in (496,48,48,16)]
fixture=(old/'rate.stdout').read_text().replace('Full-card fast decode profile','Full-card layer split profile')
for phase,n in enumerate(segments):
    fixture,count=re.subn(rf'(profile bucket{phase} )segments\d+ cycles\d+(;AFTER_CLEAN_END)',
                           rf'\g<1>segments{n} cycles{cycles[phase]}\2',fixture)
    assert count==1

with TemporaryDirectory(prefix='q38-profile-compare.') as temp:
    test=Path(temp)
    log=test/'rate.stdout'
    log.write_text(fixture)
    files={'rate.exit':'0\n','parent.exit':'0\n','rate.stderr':'','state':'STABLE041_GEN4_RESTORED\n'}
    for name,content in files.items():
        (test/name).write_text(content)
    report=compare(old,test)
    assert report['body_cycles']=={'v3':242102575794,'v4':242102575794}
    assert all(row['v3_cycles']==row['v4_cycles'] for row in report['common_group_shares'])
    ranked=report['ranked_v4_buckets']
    assert len(ranked)==13 and ranked[0]['bucket']=='matrix_total'
    assert not any(r['bucket']=='final_stop' for r in ranked)
    assert sum(r['cycles'] for r in ranked)==242102575794
    assert ranked[0]['ideal_body_factor_if_twice_as_fast']=='1.323150'
    assert report['host_rate_observations_separate_from_profile']['v4']==['0.172871238','0.172874445']
    refused=0
    def reject(a,b):
        global refused
        try:
            compare(a,b)
        except (InvalidLog,ValueError):
            refused+=1
        else:
            raise AssertionError('accepted a failed, live, or mismatched comparison')
    reject(old,old)
    for name,content in (('rate.exit','1\n'),('parent.exit','0'),('rate.stderr','unknown\n'),
                         ('state','RUNNING_LAYER_SPLIT_NO_CPU_INFERENCE\n')):
        (test/name).write_text(content);reject(old,test);(test/name).write_text(files[name])
    for broken in (fixture.replace('layer split profile','fast decode profile'),
                   fixture.replace('profile valid1','profile valid0'),
                   fixture.replace('window_start31','window_start30'),
                   fixture.replace('hex=310a','hex=300a')):
        log.write_text(broken);reject(old,test)
    assert refused==9
print('PASS closed profile comparison: archived v3, conserved synthetic v4 groups, STOP excluded, 9 refusals')

control=here/'benchmarks/qwen38-layer-split-profile-physical-20261117/remote'
raw_hash,_,actual=read_profile(control,4)
with TemporaryDirectory(prefix='q38-profile-matched-v4.') as temp:
    candidate=Path(temp)
    for name in ('rate.exit','rate.stderr','parent.exit','state'):
        (candidate/name).write_bytes((control/name).read_bytes())
    base=(control/'rate.stdout').read_text()
    log=candidate/'rate.stdout'
    log.write_text(base)
    def refuse():
        try:
            compare_v4(control,candidate)
        except (InvalidLog,ValueError):
            return
        raise AssertionError('accepted duplicate or incomplete v4 benchmark')
    refuse()
    shift=10500000
    fixture=base.replace(f'end_cycles{actual["end_cycles"]} ',f'end_cycles{actual["end_cycles"]-shift} ')
    matrix=actual['buckets'][10]['cycles']
    fixture=fixture.replace(f'bucket10 segments15376 cycles{matrix};',f'bucket10 segments15376 cycles{matrix-shift};')
    log.write_text(fixture)
    result=compare_v4(control,candidate)
    assert result['transcript_sha256']['control']==raw_hash
    assert result['body_cycles']['control']-result['body_cycles']['candidate']==shift
    assert result['final_stop_cycles']['control']==result['final_stop_cycles']['candidate']
    assert result['host_observed_tps']['bounds_relation']=='overlapping'
    assert [b['change_cycles'] for b in result['body_buckets'] if b['change_cycles']]==[-shift]
    assert next(b for b in result['body_buckets'] if b['change_cycles'])['bucket']=='matrix_total'
    for broken in (fixture.replace('window_start31','window_start30'),
                   fixture.replace('segments15376','segments15375'),
                   fixture.replace('hex=310a','hex=300a'),
                   fixture.replace(f'end_cycles{actual["end_cycles"]-shift} ',f'end_cycles{actual["end_cycles"]} ')):
        log.write_text(broken);refuse()
    log.write_text(fixture)
    (candidate/'state').write_text('RUNNING_NATIVE_Q8_NO_CPU_INFERENCE\n');refuse()
print('PASS matched v4 comparison: archived physical control and synthetic cycle delta; rates separate, 6 refusals')
