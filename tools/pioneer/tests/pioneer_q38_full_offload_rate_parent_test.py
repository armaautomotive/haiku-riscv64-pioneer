#!/usr/bin/env python3
"""Actual new rate parent over temporary files/FAKE endpoints;never hardware."""
import os
import pathlib
import runpy
import subprocess
import sys
import tempfile

here=pathlib.Path(__file__).resolve().parent
# Complete previous guarded-parent tests FIRST,not just a matching tail.
saved=sys.argv[:]
try:
    sys.argv=[str(here/'pioneer_q38_full_offload_parent_test.py'),'--batched']
    previous=runpy.run_path(sys.argv[0],run_name='__main__')
finally:sys.argv=saved
old=(here.parent/'pioneer_q38_full_offload_batched_physical.sh').read_text()
path=here.parent/'pioneer_q38_full_offload_rate_physical.sh';text=path.read_text()
subprocess.run(['sh','-n',str(path)],check=True)
functions=lambda s:s[s.index('exact(){'):s.index('test -d "$stage"')]
assert functions(text)==functions(old),'ALL exact/metadata/idle/health/atomic exchange functions must stay byte-exact'
for literal in ('--rate-32','pioneer-q38-full-metrics-20261104-a','tenstorrent_pci_probe-full-metrics90',
                'HaikuAssistantQwen27FullOffload-metrics','pioneer-q38-full-offload-rate-smoke',
                'candidate_hash=2551b50a52f6a2f7c98ca391fa1742640d5d777f23379663733679980bd643d7',
                'worker_hash=989f54c394a42c43b2bf9e0fd64844bfb378ec79fb8fbe65643e8f9bad03242a',
                'client_hash=d50896e160e4e7015ce0942ab5d83a0293a356fc90ddb0980be4313f38cfec4e',
                'native_tag=2026110400010090','stable_generation=14 native_generation=15 restored_generation=16'):
    assert text.count(literal)==1,literal
rename=lambda s:s.replace('full-offload.stdout','rate.stdout').replace('full-offload.stderr','rate.stderr').replace('full-offload.exit','rate.exit')
admission=lambda s:s[s.index('test -d "$stage"'):s.index('# FULL_OFFLOAD_PHYSICAL_FLOW_BEGIN')]
assert admission(text)==rename(admission(old)),'same full prior source/runtime/permission/idle/health admission'
tail=text[text.index('clean_tail(){'):text.index('exact(){')]
metadata=text[text.index('metadata(){'):text.index('idle(){')]
flow=text.split('# FULL_OFFLOAD_PHYSICAL_FLOW_BEGIN\n')[1].split('# FULL_OFFLOAD_PHYSICAL_FLOW_END')[0]
env={k:v for k,v in os.environ.items() if not k.startswith(('PIONEER_','TT_PIONEER_'))}
run=lambda script,extra={}:subprocess.run(['sh','-c',script],capture_output=True,text=True,env={**env,**extra})
checks=0
last=lambda available,tokens:f'PASS_FULL_OFFLOAD_RATE_JOB clean_worker_exit1 rate_available{available} output_tokens{tokens};CPU_INFERENCE_CONTEXTS0;POST_PROMPT_COUNT_WINDOW_NOT_BROAD_QUALITY_OR_ISOLATED_LATENCY'
for available in (0,1):
    for tokens in range(33):
        assert run('set -eu\n'+tail+'\nclean_tail "$FAKE_LAST"',{'FAKE_LAST':last(available,tokens)}).returncode==0;checks+=1
good=last(1,32)
for bad in ('',good+' extra',good+'\nextra',good+'\n',good.replace('rate_available1','rate_available2'),
            good.replace('output_tokens32','output_tokens33'),good.replace('output_tokens32','output_tokens-1'),
            good.replace('output_tokens32','output_tokens01'),good.replace('exit1','exit0'),
            'PASS_FULL_OFFLOAD_HELLO_FOUR clean_worker_exit1 expected_public_text1'):
    assert run('set -eu\n'+tail+'\nclean_tail "$FAKE_LAST"',{'FAKE_LAST':bad}).returncode!=0;checks+=1
base='''set -eu
stable_tag=2026102100010012;native_tag=2026110400010090
stable_generation=14;native_generation=15;restored_generation=16
fake_admin(){ printf '%s\n' "$FAKE_METADATA"; }
admin=fake_admin
'''
for role,tag,generation in (('stable-before','2026102100010012',14),('native','2026110400010090',15),('stable-after','2026102100010012',16)):
    value=f'P100A_RELOAD driver55 tag{tag} generation{generation} quiesced0 cold0 active_owner0 open_count1 guard1;CPU_METADATA_ONLY'
    script=base+metadata+'\nmetadata '+role
    assert run(script,{'FAKE_METADATA':value}).returncode==0;checks+=1
    for a,b in [('driver55','driver54'),('guard1','guard0'),('quiesced0','quiesced1'),('cold0','cold1'),
                ('active_owner0','active_owner1'),('open_count1','open_count2'),('generation'+str(generation),'generation12'),
                ('tag'+tag,'tag2026110400010089'),(';CPU_METADATA_ONLY',';OTHER')]:
        assert run(script,{'FAKE_METADATA':value.replace(a,b)}).returncode!=0;checks+=1
with tempfile.TemporaryDirectory(prefix='pioneer-full-rate-parent-',dir='/private/tmp') as temporary:
    root=pathlib.Path(temporary);stable=root/'stable-fixture';stable.write_bytes(b'FIXTURE,not driver\n')
    for case in range(14):
        stage=root/str(case);stage.mkdir();journal=stage/'journal'
        setup='''set -eu
stage="$FAKE_STAGE";stable="$FAKE_STABLE";installed=fixture-installed;candidate=fixture-native
worker=fixture-worker;client=run_fake_client;runtime=fixture-runtime;stable_hash=fixture-stable;candidate_hash=fixture-native;worker_hash=fixture-worker;client_hash=fixture-client
checks=0
exact(){ checks=$((checks+1)); [ "$checks" -ne "$FAKE_CHECK_FAILURE" ]; }
idle(){ return 0; }
metadata(){ printf 'METADATA %s\n' "$1" >> "$stage/journal"; }
health(){ printf 'HEALTH\n' >> "$stage/journal"; }
exchange(){
 printf 'EXCHANGE %s %s\n' "$1" "$2" >> "$stage/journal"
 case "$1:$2" in stable-before:native) [ "$FAKE_EXCHANGE_FAILURE" != apply ];;
 native:stable-after) [ "$FAKE_EXCHANGE_FAILURE" != restore ];; *) return 1;; esac
}
run_fake_client(){
 [ "$#" = 1 ] && [ "$1" = --full-card-rate-32 ] || return 2
 printf 'CLIENT\n' >> "$stage/journal"
 printf '%s\n' "$FAKE_LAST"
 [ "$FAKE_STDERR" = 0 ] || printf 'synthetic error\n' >&2
 return "$FAKE_CLIENT_EXIT"
}
'''
        extra={'FAKE_STAGE':str(stage),'FAKE_STABLE':str(stable),'FAKE_CHECK_FAILURE':'0','FAKE_EXCHANGE_FAILURE':'none',
               'FAKE_LAST':last(0 if case==1 else 1,8 if case==1 else 32),'FAKE_STDERR':'0','FAKE_CLIENT_EXIT':'0'}
        if 2<=case<=6:extra['FAKE_CHECK_FAILURE']=str(case-1)
        if case==7:extra['FAKE_EXCHANGE_FAILURE']='apply'
        if case==8:extra['FAKE_EXCHANGE_FAILURE']='restore'
        if case==9:extra['FAKE_CLIENT_EXIT']='1'
        if case==10:extra['FAKE_STDERR']='1'
        if case==11:extra['FAKE_LAST']='FAIL_FULL_OFFLOAD_RATE_JOB'
        if case==12:extra['FAKE_LAST']=good+' extra'
        if case==13:(stage/'.trial-once').mkdir()
        result=run(setup+tail+flow,extra)
        assert (result.returncode==0)==(case in (0,1)),(case,result.stdout,result.stderr)
        lines=journal.read_text().splitlines() if journal.exists() else []
        assert lines.count('EXCHANGE stable-before native')<=1 and lines.count('CLIENT')<=1 and lines.count('EXCHANGE native stable-after')<=1
        if case in (2,3,4,7,9,10,11,12,13):assert 'EXCHANGE native stable-after' not in lines
        if case in (0,1):
            assert lines==['EXCHANGE stable-before native','METADATA native','CLIENT','METADATA native',
                           'EXCHANGE native stable-after','METADATA stable-after','HEALTH']
            assert (stage/'state').read_text().strip()=='FULL_CARD_RATE32_CLOSED_STABLE_RESTORED'
        checks+=1
print(f'PASS actual full-card90 rate parent:{checks} typed metadata/final DATA/one-shot failure cases;ALL old54 parent cases FIRST;clean-unavailable distinct from failure;no unknown auto-restore;FAKE endpoints ONLY,no hardware')
