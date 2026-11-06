#!/usr/bin/env python3
"""Saved complete physical trial audit, not a new admission or speedup claim."""
import hashlib
import json
from pathlib import Path
import re
import sys

assert __debug__
here=Path(__file__).resolve().parent;root=here.parents[3];files=here/'remote'
assert json.loads((here/'actual-exits.json').read_text())=={
 'primary_trial_waiter':0,'independent_postflight':0,'whole_archive_copy':0,
 'whole_reader_main':0,'whole_reader_clean':0}
post=(here/'postflight.stdout').read_text();assert not (here/'postflight.stderr').read_bytes()
assert 'ALL106_TRIAL_PIDS_ABSENT_NO_MATCHING_OWNER\n' in post
assert post.endswith('PASS_INDEPENDENT_BF16_106_CLOSED_NORMAL041_GEN42_64CPU_FULLRAM_ASSISTANT_RUNTIME_PRESERVED\n')
expected={name:digest for digest,name in re.findall(
 r'^([0-9a-f]{64})  /boot/home/pioneer-q38-bf16-profile-106-a/([^/\n]+)$',post,re.M)}
assert len(expected)==14
names={'pioneer_q38_bf16_profile_full_offload_physical.sh','direct-parent.stdout','direct-parent.stderr',
 'direct-parent.exit','rate.stdout','rate.stderr','rate.exit','state','parent.pid','direct-launcher.pid'}
assert {p.name for p in files.iterdir()}==names
for name in names:assert hashlib.sha256((files/name).read_bytes()).hexdigest()==expected[name],name
for name in ('direct-parent.exit','rate.exit'):assert (files/name).read_text()=='0\n'
for name in ('direct-parent.stderr','rate.stderr'):assert not (files/name).read_bytes()
assert (files/'state').read_text()=='FULL_CARD_RATE32_CLOSED_STABLE_RESTORED\n'
script='pioneer_q38_bf16_profile_full_offload_physical.sh'
assert expected[script]=='1629ccb23e997e04449ab178984f8351eb734408ae9da7f958de6ec3e9b0fa67'
assert (files/script).read_bytes()==(root/'tools/pioneer'/script).read_bytes()
parent=(files/'direct-parent.stdout').read_text()
assert parent.endswith('PASS_FULL_CARD_RATE32_STABLE_RESTORED_NO_ASSISTANT_INSTALL_OR_SPEED_CLAIM\n')
assert parent.count('HOT_RELOAD_VERIFIED;NO_REBOOT_NO_GPU_EXECUTION\n')==2
statuses=re.findall(r'^P100A_RELOAD driver55 tag([0-9]{16}) generation([0-9]+) quiesced[01] cold0 active_owner0 open_count1 guard1;CPU_METADATA_ONLY$',parent,re.M)
assert len(statuses)==12 and {int(g) for _,g in statuses}=={40,41,42}
for tag,generation in statuses:assert tag==('2026110500010106' if generation=='41' else '2026102100010012')
assert 'generation42 quiesced0 cold0 active_owner0 open_count1 guard1;CPU_METADATA_ONLY' in post
assert len(re.findall(r'^64 .*running at',post,re.M))==1 and '/ 137034936320)' in post
for digest in ('041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403',
 'b0c851b302014e4677cee250d09634d8058be0eae223480a2aac45b3d285137f',
 'd1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229',
 '861e5287e6f417c28968422d01721c95bc2c73ed17e9111806dfa76477984a74',
 'c509da438d06f7e0dbcb23f4ef0fec2b000bfab923f5c7e86836127736d29e18',
 '0de9141eb3955cf1c993a67c80ef648b3538ce7155c17677458487ee083a4933',
 '0ae23a2c211af9fbafb89c3d93bdd136a4bdd574baeeaf2d622f582efcf426af',
 'a3b81c133641bbdacde8dd1d0e0ffa7a0a1ec0c33e95c426df1db59735379c16',
 '9cb45a36558bd78aa4d4b7ed5385cbf9df32de208f9e6c9b6d064e8527dca389',
 '69e9ec2c43a123afbb256c3feb6313cdcebdf1b2a87278ac3cdac8a1a65273d2'):
 assert re.search(r'^'+digest+r'  /boot/home/[^\n]+$',post,re.M),digest
for name in ('reader-main','reader-clean'):assert not (here/(name+'.stderr')).read_bytes()
assert (here/'reader-main.stdout').read_bytes()==(here/'reader-clean.stdout').read_bytes()
sys.path.insert(0,str(root/'tools/pioneer/tests'))
from parse_pioneer_q38_full_offload_rate_log import parse_complete,stable_text
parsed=parse_complete(stable_text(files/'rate.stdout'),stable_text(files/'rate.stderr'),phase_profile=True,bf16_profile=True)
assert parsed==json.loads((here/'reader-main.stdout').read_text().splitlines()[0])
assert (parsed['prompts'],parsed['output_tokens'],parsed['upload_rows'],parsed['uploaded_bytes'])==(31,32,2253,19399024640)
assert parsed['answer_utf8_preview']=='\n'.join(map(str,range(1,15)))
rate=parsed['observed_post_prompt_count_rate_bounds']
assert (rate['lower_tps'],rate['upper_tps'],rate['sampled_commit_steps'])==('0.078459792','0.078460452',31)
profile=parsed['on_card_phase_profile']
assert profile['valid'] and profile['committed']==62 and profile['total_cycles']==1067263476804
assert len(profile['buckets'])==14 and not profile['physical_clock_frequency_assumed'] and not parsed['broad_model_quality']
print('PASS SAVED real27B BF16/32outputs/0.078459792..0.078460452tok-s/14 physical ROOT phases/'
 'whole hashes+actual exits0+independent normal041-gen42/64CPU-fullRAM-Assistant preserved;NOT speedup or20TPS')
