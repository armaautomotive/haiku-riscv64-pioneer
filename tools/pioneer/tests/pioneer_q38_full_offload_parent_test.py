#!/usr/bin/env python3
"""Actual parent metadata/parser/flow over temporary files and FAKE endpoints."""
import os
import pathlib
import subprocess
import tempfile

here=pathlib.Path(__file__).resolve().parent
path=here.parent/'pioneer_q38_full_offload_physical.sh'
text=path.read_text();subprocess.run(['sh','-n',str(path)],check=True)
metadata=text[text.index('metadata(){'):text.index('idle(){')]
health=text[text.index('health(){'):text.index('exchange(){')]
idle=text[text.index('idle(){'):text.index('health(){')]
flow=text.split('# FULL_OFFLOAD_PHYSICAL_FLOW_BEGIN\n')[1].split('# FULL_OFFLOAD_PHYSICAL_FLOW_END')[0]
last='PASS_FULL_OFFLOAD_HELLO_FOUR clean_worker_exit1 expected_public_text1;CPU_INFERENCE_CONTEXTS0;NOT_STEADY_TOKEN_RATE_OR_BROAD_MODEL_QUALITY'
env={k:v for k,v in os.environ.items() if not k.startswith(('PIONEER_','TT_PIONEER_'))}
base='''set -eu
stable_tag=2026102100010012;native_tag=2026110400010088
stable_generation=10;native_generation=11;restored_generation=12
fake_admin(){ printf '%s\n' "$FAKE_METADATA"; }
admin=fake_admin
'''
def run(script,extra=None):
    return subprocess.run(['sh','-c',script],stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True,env={**env,**(extra or {})})
checks=0
for role,tag,generation in (('stable-before','2026102100010012',10),('native','2026110400010088',11),('stable-after','2026102100010012',12)):
    value=f'P100A_RELOAD driver55 tag{tag} generation{generation} quiesced0 cold0 active_owner0 open_count1 guard1;CPU_METADATA_ONLY'
    script=base+metadata+'\nmetadata '+role+'\n'
    assert run(script,{'FAKE_METADATA':value}).returncode==0;checks+=1
    for old,new in [('driver55','driver54'),('guard1','guard0'),('quiesced0','quiesced1'),('cold0','cold1'),
                    ('active_owner0','active_owner1'),('open_count1','open_count2'),('generation'+str(generation),'generation9'),
                    ('tag'+tag,'tag2026103100010075'),(';CPU_METADATA_ONLY',';OTHER')]:
        assert run(script,{'FAKE_METADATA':value.replace(old,new)}).returncode!=0;checks+=1
for args in ('','unknown','native extra','7'):
    assert run(base+metadata+'\nmetadata '+args+'\n',{'FAKE_METADATA':''}).returncode!=0;checks+=1
good_health='Kernel name: fixture\n64 (Unknown), revision 0000 running at 1000MHz\n130000000000 bytes free (used/max 7034936320 / 137034936320)'
for value,expected in ((good_health,0),(good_health.replace('64 (','1 ('),1),
                       (good_health.replace('137034936320','3187671040'),1)):
    result=run('set -eu\nsysinfo(){ printf "%s\\n" "$FAKE_HEALTH"; }\n'+health+'\nhealth',{'FAKE_HEALTH':value})
    assert bool(result.returncode)==bool(expected);checks+=1
for value,expected in [('   Id Team\n 1 kernel_team\n 11 /boot/system/Deskbar',0),
                       ('   Id Team\n 1 kernel_team\n 12 /boot/home/HaikuAssistantQwen27FullOffload',1),
                       ('   Id Team\n 1 kernel_team\n 12 /boot/home/pioneer-q38-full-offload-smoke',1),
                       ('broken data',1)]:
    result=run('set -eu\nps(){ printf "%s\\n" "$FAKE_PS"; }\n'+idle+'\nidle',{'FAKE_PS':value})
    assert bool(result.returncode)==bool(expected);checks+=1
with tempfile.TemporaryDirectory(prefix='pioneer-full-offload-parent-',dir='/private/tmp') as temporary:
    root=pathlib.Path(temporary);stable=root/'stable-fixture';stable.write_bytes(b'FIXTURE,not a real driver\n')
    for case in range(13):
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
 [ "$#" = 1 ] && [ "$1" = --full-card-hello-four ] || return 2
 printf 'CLIENT\n' >> "$stage/journal"
 printf '%s\n' "$FAKE_LAST"
 [ "$FAKE_STDERR" = 0 ] || printf 'synthetic error\n' >&2
 return "$FAKE_CLIENT_EXIT"
}
'''
        extra={'FAKE_STAGE':str(stage),'FAKE_STABLE':str(stable),'FAKE_CHECK_FAILURE':'0','FAKE_EXCHANGE_FAILURE':'none',
               'FAKE_LAST':last,'FAKE_STDERR':'0','FAKE_CLIENT_EXIT':'0'}
        if 1<=case<=5:extra['FAKE_CHECK_FAILURE']=str(case)
        if case==6:extra['FAKE_EXCHANGE_FAILURE']='apply'
        if case==7:extra['FAKE_EXCHANGE_FAILURE']='restore'
        if case==8:extra['FAKE_CLIENT_EXIT']='1'
        if case==9:extra['FAKE_STDERR']='1'
        if case==10:extra['FAKE_LAST']='FAIL_FULL_OFFLOAD_HELLO_FOUR'
        if case==11:extra['FAKE_LAST']=last+' extra'
        if case==12:(stage/'.trial-once').mkdir()
        result=run(setup+flow,extra);assert (result.returncode==0)==(case==0),(case,result.stdout,result.stderr)
        lines=journal.read_text().splitlines() if journal.exists() else []
        assert lines.count('EXCHANGE stable-before native')<=1 and lines.count('CLIENT')<=1 and lines.count('EXCHANGE native stable-after')<=1
        if case in (1,6,12):assert 'CLIENT' not in lines
        if case in (1,2,3,4,6,8,9,10,11,12):assert 'EXCHANGE native stable-after' not in lines
        if case==0:
            assert lines==['EXCHANGE stable-before native','METADATA native','CLIENT','METADATA native',
                           'EXCHANGE native stable-after','METADATA stable-after','HEALTH']
            assert (stage/'state').read_text().strip()=='FULL_CARD_HELLO_FOUR_CLOSED_STABLE_RESTORED'
        checks+=1
print(f'PASS actual full-card parent: {checks} typed-metadata/idle/64CPU+RAM/one-shot-flow cases;unknown child/stderr/text/identity failures never auto-restore;FAKE endpoints ONLY,no hardware exchange')
